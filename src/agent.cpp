//
// Copyright Copyright 2009-2022, AMT – The Association For Manufacturing Technology (“AMT”)
// All rights reserved.
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//

#include "agent.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/adaptor/sliced.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/any_range.hpp>
#include <boost/range/functions.hpp>
#include <boost/range/metafunctions.hpp>
#include <boost/range/numeric.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <algorithm>
#include <chrono>
#include <fcntl.h>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <thread>

#include "asset/asset.hpp"
#include "asset/cutting_tool.hpp"
#include "asset/file_asset.hpp"
#include "asset/qif_document.hpp"
#include "asset/raw_material.hpp"
#include "configuration/config_options.hpp"
#include "device_model/agent_device.hpp"
#include "entity/xml_parser.hpp"
#include "logging.hpp"
#include "observation/observation.hpp"
#include "printer/json_printer.hpp"
#include "printer/xml_printer.hpp"
#include "sink/rest_sink/file_cache.hpp"
#include "sink/rest_sink/session.hpp"
#include "version.h"

using namespace std;

namespace mtconnect {
  using namespace device_model;
  using namespace data_item;
  using namespace entity;
  using namespace sink::rest_sink;
  namespace net = boost::asio;
  namespace fs = boost::filesystem;
  namespace config = mtconnect::configuration;

  static const string g_unavailable("UNAVAILABLE");
  static const string g_available("AVAILABLE");

  // Agent public methods
  Agent::Agent(boost::asio::io_context &context, const string &deviceXmlPath,
               const ConfigOptions &options)
    : m_options(options),
      m_context(context),
      m_strand(m_context),
      m_xmlParser(make_unique<parser::XmlParser>()),
      m_schemaVersion(GetOption<string>(options, config::SchemaVersion)),
      m_deviceXmlPath(deviceXmlPath),
      m_circularBuffer(GetOption<int>(options, config::BufferSize).value_or(17),
                       GetOption<int>(options, config::CheckpointFrequency).value_or(1000)),
      m_pretty(GetOption<bool>(options, mtconnect::configuration::Pretty).value_or(false))
  {
    using namespace asset;

    CuttingToolArchetype::registerAsset();
    CuttingTool::registerAsset();
    FileArchetypeAsset::registerAsset();
    FileAsset::registerAsset();
    RawMaterial::registerAsset();
    QIFDocumentWrapper::registerAsset();

    m_assetStorage = make_unique<AssetBuffer>(
        GetOption<int>(options, mtconnect::configuration::MaxAssets).value_or(1024));
    m_versionDeviceXml =
        GetOption<bool>(options, mtconnect::configuration::VersionDeviceXmlUpdates).value_or(false);

    auto jsonVersion =
        uint32_t(GetOption<int>(options, mtconnect::configuration::JsonVersion).value_or(2));

    // Create the Printers
    m_printers["xml"] = make_unique<printer::XmlPrinter>(m_pretty);
    m_printers["json"] = make_unique<printer::JsonPrinter>(jsonVersion, m_pretty);

    if (m_schemaVersion)
    {
      for (auto &[k, pr] : m_printers)
        pr->setSchemaVersion(*m_schemaVersion);
    }
  }

  void Agent::initialize(pipeline::PipelineContextPtr context)
  {
    NAMED_SCOPE("Agent::initialize");

    m_pipelineContext = context;
    m_loopback =
        std::make_shared<source::LoopbackSource>("AgentSource", m_strand, context, m_options);

    auto devices = loadXMLDeviceFile(m_deviceXmlPath);
    if (!m_schemaVersion)
    {
      m_schemaVersion.emplace(StrDefaultSchemaVersion());
    }

    auto version = IntSchemaVersion(*m_schemaVersion);
    for (auto &[k, pr] : m_printers)
      pr->setSchemaVersion(*m_schemaVersion);

    auto disableAgentDevice = GetOption<bool>(m_options, config::DisableAgentDevice);
    if (!(disableAgentDevice && *disableAgentDevice) && version >= SCHEMA_VERSION(1, 7))
    {
      createAgentDevice();
    }

    // For the DeviceAdded event for each device
    for (auto device : devices)
      addDevice(device);

    loadCachedProbe();
    m_initialized = true;
  }

  void Agent::initialDataItemObservations()
  {
    NAMED_SCOPE("Agent::initialDataItemObservations");

    if (!m_observationsInitialized)
    {
      for (auto device : m_deviceIndex)
        initializeDataItems(device);

      if (m_agentDevice)
      {
        for (auto device : m_deviceIndex)
        {
          auto d = m_agentDevice->getDeviceDataItem("device_added");
          string uuid = *device->getUuid();
          m_loopback->receive(d, uuid);
        }
      }

      m_observationsInitialized = true;
    }
  }

  Agent::~Agent()
  {
    m_xmlParser.reset();
    m_sinks.clear();
    m_sources.clear();
    m_agentDevice = nullptr;
  }

  void Agent::start()
  {
    NAMED_SCOPE("Agent::start");
    try
    {
      for (auto sink : m_sinks)
        sink->start();

      initialDataItemObservations();

      if (m_agentDevice)
      {
        auto d = m_agentDevice->getDeviceDataItem("agent_avail");
        m_loopback->receive(d, "AVAILABLE"s);
      }

      // Start all the sources
      for (auto source : m_sources)
        source->start();
    }
    catch (std::runtime_error &e)
    {
      LOG(fatal) << "Cannot start server: " << e.what();
      std::exit(1);
    }
  }

  void Agent::stop()
  {
    NAMED_SCOPE("Agent::stop");

    // Stop all adapter threads...
    LOG(info) << "Shutting down sources";
    for (auto source : m_sources)
      source->stop();

    LOG(info) << "Shutting down sinks";
    for (auto sink : m_sinks)
      sink->stop();

    // Signal all observers
    LOG(info) << "Signaling observers to close sessions";
    for (auto di : m_dataItemMap)
    {
      auto ldi = di.second.lock();
      if (ldi)
        ldi->signalObservers(0);
    }

    LOG(info) << "Shutting down completed";
  }

  // ---------------------------------------
  // Pipeline methods
  // ---------------------------------------
  void Agent::receiveObservation(observation::ObservationPtr observation)
  {
    std::lock_guard<buffer::CircularBuffer> lock(m_circularBuffer);
    if (m_circularBuffer.addToBuffer(observation) != 0)
    {
      for (auto &sink : m_sinks)
        sink->publish(observation);
    }
  }

  void Agent::receiveAsset(asset::AssetPtr asset)
  {
    DevicePtr device;
    auto uuid = asset->getDeviceUuid();
    if (uuid)
      device = findDeviceByUUIDorName(*uuid);
    else
      device = defaultDevice();

    if (device && device->getAssetChanged() && device->getAssetRemoved())
    {
      if (asset->getDeviceUuid() && *asset->getDeviceUuid() != *device->getUuid())
      {
        asset->setProperty("deviceUuid", *device->getUuid());
      }

      string aid = asset->getAssetId();
      if (aid[0] == '@')
      {
        if (aid.empty())
          aid = asset->getAssetId();
        aid.erase(0, 1);
        aid.insert(0, *device->getUuid());
      }
      if (aid != asset->getAssetId())
      {
        asset->setAssetId(aid);
      }
    }

    m_assetStorage->addAsset(asset);

    for (auto &sink : m_sinks)
      sink->publish(asset);

    if (device)
    {
      DataItemPtr di;
      if (asset->isRemoved())
        di = device->getAssetRemoved();
      else
        di = device->getAssetChanged();
      if (di)
      {
        m_loopback->receive(di, {{"assetType", asset->getName()}, {"VALUE", asset->getAssetId()}});
      }

      updateAssetCounts(device, asset->getType());
    }
  }

  bool Agent::reloadDevices(const std::string &deviceFile)
  {
    try
    {
      // Load the configuration for the Agent
      auto devices = m_xmlParser->parseFile(
          deviceFile, dynamic_cast<printer::XmlPrinter *>(m_printers["xml"].get()));

      if (m_xmlParser->getSchemaVersion() &&
          IntSchemaVersion(*m_xmlParser->getSchemaVersion()) != IntSchemaVersion(*m_schemaVersion))
      {
        LOG(info) << "Got version: " << *(m_xmlParser->getSchemaVersion());
        LOG(warning) << "Schema version does not match agent schema version, restarting the agent";
        return false;
      }

      // Fir the DeviceAdded event for each device
      bool changed = false;
      for (auto device : devices)
      {
        changed = receiveDevice(device, false) || changed;
      }
      if (changed)
        loadCachedProbe();

      return true;
    }
    catch (runtime_error &e)
    {
      LOG(fatal) << "Error loading xml configuration: " + deviceFile;
      LOG(fatal) << "Error detail: " << e.what();
      cerr << e.what() << endl;
      throw e;
    }
    catch (exception &f)
    {
      LOG(fatal) << "Error loading xml configuration: " + deviceFile;
      LOG(fatal) << "Error detail: " << f.what();
      cerr << f.what() << endl;
      throw f;
    }
  }

  bool Agent::receiveDevice(device_model::DevicePtr device, bool version)
  {
    NAMED_SCOPE("Agent::receiveDevice");

    // diff the device against the current device with the same uuid
    auto uuid = device->getUuid();
    if (!uuid)
    {
      LOG(error) << "Device does not have a uuid: " << device->getName();
      return false;
    }

    DevicePtr oldDev = findDeviceByUUIDorName(*uuid);
    if (!oldDev)
    {
      auto name = device->getComponentName();
      if (!name)
      {
        LOG(error) << "Device does not have a name" << *uuid;
        return false;
      }

      oldDev = findDeviceByUUIDorName(*name);
    }

    // If this is a new device
    if (!oldDev)
    {
      LOG(info) << "Received new device: " << *uuid << ", adding";
      addDevice(device);
      if (version)
        versionDeviceXml();
      return true;
    }
    else
    {
      auto name = device->getComponentName();
      if (!name)
      {
        LOG(error) << "Device does not have a name" << *device->getUuid();
        return false;
      }

      // if different,  and revise to new device leaving in place
      // the asset changed, removed, and availability data items
      ErrorList errors;
      if (auto odi = oldDev->getAssetChanged(), ndi = device->getAssetChanged(); odi && !ndi)
        device->addDataItem(odi, errors);
      if (auto odi = oldDev->getAssetRemoved(), ndi = device->getAssetRemoved(); odi && !ndi)
        device->addDataItem(odi, errors);
      if (auto odi = oldDev->getAvailability(), ndi = device->getAvailability(); odi && !ndi)
        device->addDataItem(odi, errors);
      if (auto odi = oldDev->getAssetCount(), ndi = device->getAssetCount(); odi && !ndi)
        device->addDataItem(odi, errors);

      verifyDevice(device);

      LOG(info) << "Checking if device " << *uuid << " has changed";
      if (*device != *oldDev)
      {
        LOG(info) << "Device " << *uuid << " changed, updating model";

        // Remove the old data items
        set<string> skip;
        for (auto &di : oldDev->getDeviceDataItems())
        {
          if (!di.expired())
          {
            m_dataItemMap.erase(di.lock()->getId());
            skip.insert(di.lock()->getId());
          }
        }

        // Replace device in device maps
        auto it = find(m_deviceIndex.begin(), m_deviceIndex.end(), oldDev);
        if (it != m_deviceIndex.end())
          m_deviceIndex.replace(it, device);
        else
        {
          LOG(error) << "Cannot find Device " << *uuid << " in devices";
          return false;
        }

        initializeDataItems(device, skip);

        LOG(info) << "Device " << *uuid << " updating circular buffer";
        m_circularBuffer.updateDataItems(m_dataItemMap);

        if (version)
          versionDeviceXml();

        if (m_agentDevice)
        {
          auto d = m_agentDevice->getDeviceDataItem("device_changed");
          m_loopback->receive(d, *uuid);
        }

        return true;
      }
      else
      {
        LOG(info) << "Device " << *uuid << " did not change, ignoring new device";
      }
    }

    return false;
  }

  void Agent::versionDeviceXml()
  {
    if (m_versionDeviceXml)
    {
      // update with a new version of the device.xml, saving the old one
      // with a date time stamp
      auto ext = "."s + getCurrentTime(LOCAL);
      fs::path file(m_deviceXmlPath);
      fs::path backup(m_deviceXmlPath + ext);
      if (!fs::exists(backup))
        fs::rename(file, backup);

      printer::XmlPrinter printer(true);

      std::list<DevicePtr> list;
      copy_if(m_deviceIndex.begin(), m_deviceIndex.end(), back_inserter(list),
              [](DevicePtr d) { return dynamic_cast<AgentDevice *>(d.get()) == nullptr; });
      auto probe = printer.printProbe(0, 0, 0, 0, 0, list);

      ofstream devices(file.string());
      devices << probe;
      devices.close();
    }
  }

  bool Agent::removeAsset(DevicePtr device, const std::string &id,
                          const std::optional<Timestamp> time)
  {
    auto asset = m_assetStorage->removeAsset(id);
    if (asset)
    {
      for (auto &sink : m_sinks)
        sink->publish(asset);

      notifyAssetRemoved(device, asset);
      updateAssetCounts(device, asset->getType());

      return true;
    }
    else
    {
      return false;
    }
  }

  bool Agent::removeAllAssets(const std::optional<std::string> device,
                              const std::optional<std::string> type,
                              const std::optional<Timestamp> time, asset::AssetList &list)
  {
    std::optional<std::string> uuid;
    DevicePtr dev;
    if (device)
    {
      dev = findDeviceByUUIDorName(*device);
      if (dev)
        uuid = dev->getUuid();
      else
        uuid = device;
    }

    auto count = m_assetStorage->removeAll(list, uuid, type, time);
    for (auto &asset : list)
    {
      notifyAssetRemoved(nullptr, asset);
    }

    if (dev)
    {
      updateAssetCounts(dev, type);
    }
    else
    {
      for (auto d : m_deviceIndex)
        updateAssetCounts(d, type);
    }

    return count > 0;
  }

  void Agent::notifyAssetRemoved(DevicePtr device, const asset::AssetPtr &asset)
  {
    if (device || asset->getDeviceUuid())
    {
      auto dev = device;
      if (!device)
      {
        auto &idx = m_deviceIndex.get<ByUuid>();
        auto it = idx.find(*asset->getDeviceUuid());
        if (it != idx.end())
          dev = *it;
      }
      if (dev && dev->getAssetRemoved())
      {
        m_loopback->receive(dev->getAssetRemoved(),
                            {{"assetType", asset->getName()}, {"VALUE", asset->getAssetId()}});

        auto changed = dev->getAssetChanged();
        auto last = getLatest(changed);
        if (last && asset->getAssetId() == last->getValue<string>())
        {
          m_loopback->receive(changed, {{"assetType", asset->getName()}, {"VALUE", g_unavailable}});
        }
      }
    }
  }

  // ---------------------------------------
  // Agent Device
  // ---------------------------------------

  void Agent::createAgentDevice()
  {
    NAMED_SCOPE("Agent::createAgentDevice");

    using namespace boost;
    using namespace asio;
    using res = ip::udp::resolver;

    auto port = GetOption<int>(m_options, mtconnect::configuration::Port).value_or(5000);
    auto service = boost::lexical_cast<string>(port);
    string address;

    boost::system::error_code ec;
    res resolver(m_context);
    auto iter = resolver.resolve(ip::host_name(), service, res::flags::address_configured, ec);
    if (ec)
    {
      LOG(warning) << "Cannot find IP address: " << ec.message();
      address = "127.0.0.1";
    }
    else
    {
      res::iterator end;
      while (iter != end)
      {
        const auto &ep = iter->endpoint();
        const auto &ad = ep.address();
        if (!ad.is_unspecified() && !ad.is_loopback())
        {
          auto ads {ad.to_string()};
          if (ads.length() > address.length() ||
              (ads.length() == address.length() && ads > address))
          {
            address = ads;
          }
        }
        iter++;
      }
    }

    address.append(":").append(service);

    uuids::name_generator_latest gen(uuids::ns::dns());
    auto uuid = uuids::to_string(gen(address));
    auto id = "agent_"s + uuid.substr(0, uuid.find_first_of('-'));

    // Create the Agent Device
    ErrorList errors;
    Properties ps {
        {"uuid", uuid}, {"id", id}, {"name", "Agent"s}, {"mtconnectVersion", *m_schemaVersion}};
    m_agentDevice =
        dynamic_pointer_cast<AgentDevice>(AgentDevice::getFactory()->make("Agent", ps, errors));
    if (!errors.empty())
    {
      for (auto &e : errors)
        LOG(fatal) << "Error creating the agent device: " << e->what();
      throw EntityError("Cannot create AgentDevice");
    }
    addDevice(m_agentDevice);
  }

  // ----------------------------------------------
  // Device management and Initialization
  // ----------------------------------------------

  std::list<device_model::DevicePtr> Agent::loadXMLDeviceFile(const std::string &configXmlPath)
  {
    NAMED_SCOPE("Agent::loadXMLDeviceFile");

    try
    {
      // Load the configuration for the Agent
      auto devices = m_xmlParser->parseFile(
          configXmlPath, dynamic_cast<printer::XmlPrinter *>(m_printers["xml"].get()));

      if (!m_schemaVersion && m_xmlParser->getSchemaVersion())
      {
        m_schemaVersion = m_xmlParser->getSchemaVersion();
      }
      else if (!m_schemaVersion && !m_xmlParser->getSchemaVersion())
      {
        m_schemaVersion = StrDefaultSchemaVersion();
      }

      return devices;
    }
    catch (runtime_error &e)
    {
      LOG(fatal) << "Error loading xml configuration: " + configXmlPath;
      LOG(fatal) << "Error detail: " << e.what();
      cerr << e.what() << endl;
      throw e;
    }
    catch (exception &f)
    {
      LOG(fatal) << "Error loading xml configuration: " + configXmlPath;
      LOG(fatal) << "Error detail: " << f.what();
      cerr << f.what() << endl;
      throw f;
    }

    return {};
  }

  void Agent::verifyDevice(DevicePtr device)
  {
    NAMED_SCOPE("Agent::verifyDevice");

    auto version = IntSchemaVersion(*m_schemaVersion);

    // Add the devices to the device map and create availability and
    // asset changed events if they don't exist
    // Make sure we have two device level data items:
    // 1. Availability
    // 2. AssetChanged
    if (!device->getAvailability())
    {
      // Create availability data item and add it to the device.
      entity::ErrorList errors;
      auto di = DataItem::make(
          {{"type", "AVAILABILITY"s}, {"id", device->getId() + "_avail"}, {"category", "EVENT"s}},
          errors);
      device->addDataItem(di, errors);
    }

    if (!device->getAssetChanged() && version >= SCHEMA_VERSION(1, 2))
    {
      entity::ErrorList errors;
      // Create asset change data item and add it to the device.
      auto di = DataItem::make({{"type", "ASSET_CHANGED"s},
                                {"id", device->getId() + "_asset_chg"},
                                {"category", "EVENT"s}},
                               errors);
      device->addDataItem(di, errors);
    }

    if (device->getAssetChanged() && version >= SCHEMA_VERSION(1, 5))
    {
      auto di = device->getAssetChanged();
      if (!di->isDiscrete())
        di->makeDiscrete();
    }

    if (!device->getAssetRemoved() && version >= SCHEMA_VERSION(1, 3))
    {
      // Create asset removed data item and add it to the device.
      entity::ErrorList errors;
      auto di = DataItem::make({{"type", "ASSET_REMOVED"s},
                                {"id", device->getId() + "_asset_rem"},
                                {"category", "EVENT"s}},
                               errors);
      device->addDataItem(di, errors);
    }

    if (!device->getAssetCount() && version >= SCHEMA_VERSION(2, 0))
    {
      entity::ErrorList errors;
      auto di = DataItem::make({{"type", "ASSET_COUNT"s},
                                {"id", device->getId() + "_asset_count"},
                                {"category", "EVENT"s},
                                {"representation", "DATA_SET"s}},
                               errors);
      device->addDataItem(di, errors);
    }
  }

  void Agent::initializeDataItems(DevicePtr device, std::optional<std::set<std::string>> skip)
  {
    NAMED_SCOPE("Agent::initializeDataItems");

    // Grab data from configuration
    string time = getCurrentTime(GMT_UV_SEC);

    // Initialize the id mapping for the devices and set all data items to UNAVAILABLE
    for (auto item : device->getDeviceDataItems())
    {
      if (item.expired())
        continue;

      auto d = item.lock();
      if ((!skip || skip->count(d->getId()) > 0) && m_dataItemMap.count(d->getId()) > 0)
      {
        auto di = m_dataItemMap[d->getId()].lock();
        if (di && di != d)
        {
          LOG(fatal) << "Duplicate DataItem id " << d->getId()
                     << " for device: " << *device->getComponentName();
          std::exit(1);
        }
      }
      else
      {
        // Check for single valued constrained data items.
        const string *value = &g_unavailable;
        if (d->isCondition())
          value = &g_unavailable;
        else if (d->getConstantValue())
          value = &d->getConstantValue().value();

        m_loopback->receive(d, *value);
        m_dataItemMap[d->getId()] = d;
      }
    }
  }

  // Add the a device from a configuration file
  void Agent::addDevice(DevicePtr device)
  {
    NAMED_SCOPE("Agent::addDevice");

    // Check if device already exists
    string uuid = *device->getUuid();
    auto &idx = m_deviceIndex.get<ByUuid>();
    auto old = idx.find(uuid);
    if (old != idx.end())
    {
      // Update existing device
      LOG(fatal) << "Device " << *device->getUuid() << " already exists. "
                 << " Update not supported yet";
      std::exit(1);
    }
    else
    {
      // Check if we are already initialized. If so, the device will need to be
      // verified, additional data items added, and initial values set.
      // if (!m_initialized)
      {
        m_deviceIndex.push_back(device);

        // TODO: Redo Resolve Reference  with entity
        // device->resolveReferences();
        verifyDevice(device);

        if (m_observationsInitialized)
        {
          initializeDataItems(device);

          // Check for single valued constrained data items.
          if (m_agentDevice && device != m_agentDevice)
          {
            auto d = m_agentDevice->getDeviceDataItem("device_added");
            m_loopback->receive(d, uuid);
          }
        }
      }
      // else
      //   LOG(warning) << "Adding device " << uuid << " after initialialization not supported yet";
    }

    for (auto &printer : m_printers)
      printer.second->setModelChangeTime(getCurrentTime(GMT_UV_SEC));
  }

  void Agent::deviceChanged(DevicePtr device, const std::string &oldUuid,
                            const std::string &oldName)
  {
    NAMED_SCOPE("Agent::deviceChanged");

    bool changed = false;
    string uuid = *device->getUuid();
    if (uuid != oldUuid)
    {
      changed = true;
      if (m_agentDevice)
      {
        auto d = m_agentDevice->getDeviceDataItem("device_removed");
        if (d)
          m_loopback->receive(d, oldUuid);
      }
    }

    if (*device->getComponentName() != oldName)
    {
      changed = true;
    }

    if (changed)
    {
      versionDeviceXml();
      loadCachedProbe();

      if (m_agentDevice)
      {
        for (auto &printer : m_printers)
          printer.second->setModelChangeTime(getCurrentTime(GMT_UV_SEC));

        if (device->getUuid() != oldUuid)
        {
          auto d = m_agentDevice->getDeviceDataItem("device_added");
          if (d)
            m_loopback->receive(d, uuid);
        }
        else
        {
          auto d = m_agentDevice->getDeviceDataItem("device_changed");
          if (d)
            m_loopback->receive(d, uuid);
        }
      }
    }
  }

  void Agent::loadCachedProbe()
  {
    NAMED_SCOPE("Agent::loadCachedProbe");

    // Reload the document for path resolution
    auto xmlPrinter = dynamic_cast<printer::XmlPrinter *>(m_printers["xml"].get());
    m_xmlParser->loadDocument(xmlPrinter->printProbe(0, 0, 0, 0, 0, getDevices()));

    for (auto &printer : m_printers)
      printer.second->setModelChangeTime(getCurrentTime(GMT_UV_SEC));
  }

  // ----------------------------------------------------
  // Helper Methods
  // ----------------------------------------------------

  DevicePtr Agent::getDeviceByName(const std::string &name) const
  {
    if (name.empty())
      return defaultDevice();

    auto &idx = m_deviceIndex.get<ByName>();
    auto devPos = idx.find(name);
    if (devPos != idx.end())
      return *devPos;

    return nullptr;
  }

  DevicePtr Agent::getDeviceByName(const std::string &name)
  {
    if (name.empty())
      return defaultDevice();

    auto &idx = m_deviceIndex.get<ByName>();
    auto devPos = idx.find(name);
    if (devPos != idx.end())
      return *devPos;

    return nullptr;
  }

  DevicePtr Agent::findDeviceByUUIDorName(const std::string &idOrName) const
  {
    if (idOrName.empty())
      return defaultDevice();

    DevicePtr res;
    if (auto d = m_deviceIndex.get<ByUuid>().find(idOrName); d != m_deviceIndex.get<ByUuid>().end())
      res = *d;
    else if (auto d = m_deviceIndex.get<ByName>().find(idOrName);
             d != m_deviceIndex.get<ByName>().end())
      res = *d;

    return res;
  }

  // ----------------------------------------------------
  // Adapter Methods
  // ----------------------------------------------------

  void Agent::addSource(source::SourcePtr source, bool start)
  {
    m_sources.emplace_back(source);

    if (start)
      source->start();

    auto adapter = dynamic_pointer_cast<source::adapter::Adapter>(source);
    if (m_agentDevice && adapter)
    {
      m_agentDevice->addAdapter(adapter);

      if (m_observationsInitialized)
        initializeDataItems(m_agentDevice);

      // Reload the document for path resolution
      if (m_initialized)
      {
        loadCachedProbe();
      }
    }
  }

  void Agent::addSink(sink::SinkPtr sink, bool start)
  {
    m_sinks.emplace_back(sink);

    if (start)
      sink->start();
  }

  void AgentPipelineContract::deliverConnectStatus(entity::EntityPtr entity,
                                                   const StringList &devices, bool autoAvailable)
  {
    auto value = entity->getValue<string>();
    if (value == "CONNECTING")
    {
      m_agent->connecting(entity->get<string>("source"));
    }
    else if (value == "CONNECTED")
    {
      m_agent->connected(entity->get<string>("source"), devices, autoAvailable);
    }
    else if (value == "DISCONNECTED")
    {
      m_agent->disconnected(entity->get<string>("source"), devices, autoAvailable);
    }
    else
    {
      LOG(error) << "Unexpected connection status received: " << value;
    }
  }

  void AgentPipelineContract::deliverCommand(entity::EntityPtr entity)
  {
    static auto pattern = regex("\\*[ ]*([^:]+):[ ]*(.+)");
    auto value = entity->getValue<string>();
    smatch match;

    if (std::regex_match(value, match, pattern))
    {
      auto device = entity->maybeGet<string>("device");
      auto command = boost::algorithm::to_lower_copy(match[1].str());
      auto param = match[2].str();
      auto source = entity->maybeGet<string>("source");

      if (!device || !source)
      {
        LOG(error) << "Invalid command: " << command << ", device or source not specified";
      }
      else
      {
        LOG(debug) << "Processing command: " << command << ": " << value;
        m_agent->receiveCommand(*device, command, param, *source);
      }
    }
    else
    {
      LOG(warning) << "Cannot parse command: " << value;
    }
  }

  void Agent::connecting(const std::string &adapter)
  {
    if (m_agentDevice)
    {
      auto di = m_agentDevice->getConnectionStatus(adapter);
      if (di)
        m_loopback->receive(di, "LISTENING");
    }
  }

  // Add values for related data items UNAVAILABLE
  void Agent::disconnected(const std::string &adapter, const StringList &devices,
                           bool autoAvailable)
  {
    LOG(debug) << "Disconnected from adapter, setting all values to UNAVAILABLE";

    if (m_agentDevice)
    {
      auto di = m_agentDevice->getConnectionStatus(adapter);
      if (di)
        m_loopback->receive(di, "CLOSED");
    }

    for (auto &name : devices)
    {
      DevicePtr device = findDeviceByUUIDorName(name);
      if (device == nullptr)
      {
        LOG(warning) << "Cannot find device " << name << " when adapter " << adapter
                     << "disconnected";
        continue;
      }

      for (auto di : device->getDeviceDataItems())
      {
        auto dataItem = di.lock();
        if (dataItem && ((dataItem->getDataSource() && *dataItem->getDataSource() == adapter) ||
                         (autoAvailable && !dataItem->getDataSource() &&
                          dataItem->getType() == "AVAILABILITY")))
        {
          auto ptr = getLatest(dataItem->getId());

          if (ptr)
          {
            const string *value = nullptr;
            if (dataItem->getConstantValue())
              value = &dataItem->getConstantValue().value();
            else if (!ptr->isUnavailable())
              value = &g_unavailable;

            if (value)
              m_loopback->receive(dataItem, *value);
          }
        }
        else if (!dataItem)
          LOG(warning) << "Free data item found in device data items";
      }
    }
  }

  void Agent::connected(const std::string &adapter, const StringList &devices, bool autoAvailable)
  {
    if (m_agentDevice)
    {
      auto di = m_agentDevice->getConnectionStatus(adapter);
      if (di)
        m_loopback->receive(di, "ESTABLISHED");
    }

    if (!autoAvailable)
      return;

    for (auto &name : devices)
    {
      DevicePtr device = findDeviceByUUIDorName(name);
      if (device == nullptr)
      {
        LOG(warning) << "Cannot find device " << name << " when adapter " << adapter << "connected";
        continue;
      }
      LOG(debug) << "Connected to adapter, setting all Availability data items to AVAILABLE";

      if (auto avail = device->getAvailability())
      {
        LOG(debug) << "Adding availabilty event for " << device->getAvailability()->getId();
        m_loopback->receive(avail, g_available);
      }
      else
        LOG(debug) << "Cannot find availability for " << *device->getComponentName();
    }
  }

  void Agent::sourceFailed(const std::string &identity)
  {
    auto source = findSource(identity);
    if (source)
    {
      source->stop();
      m_sources.remove(source);

      bool ext = false;
      for (auto &s : m_sources)
      {
        if (!s->isLoopback())
        {
          ext = true;
          break;
        }
      }

      if (!ext)
      {
        LOG(fatal) << "Source " << source->getName() << " failed";
        LOG(fatal) << "No external adapters present, shutting down";
        stop();
        m_context.stop();
      }
      else
      {
        LOG(error) << "Source " << source->getName() << " failed";
      }
    }
    else
    {
      LOG(error) << "Cannot find failed source: " << source->getName();
    }
  }

  // -----------------------------------------------
  // Validation methods
  // -----------------------------------------------

  string Agent::devicesAndPath(const std::optional<string> &path, const DevicePtr device) const
  {
    string dataPath;

    if (device)
    {
      string prefix;
      if (device->getName() == "Agent")
        prefix = "//Devices/Agent";
      else
        prefix = "//Devices/Device[@uuid=\"" + *device->getUuid() + "\"]";

      if (path)
      {
        stringstream parse(*path);
        string token;

        // Prefix path (i.e. "path1|path2" => "{prefix}path1|{prefix}path2")
        while (getline(parse, token, '|'))
          dataPath += prefix + token + "|";

        dataPath.erase(dataPath.length() - 1);
      }
      else
      {
        dataPath = prefix;
      }
    }
    else
    {
      dataPath = path ? *path : "//Devices/Device|//Devices/Agent";
    }

    return dataPath;
  }

  void AgentPipelineContract::deliverAssetCommand(entity::EntityPtr command)
  {
    const std::string &cmd = command->getValue<string>();
    if (cmd == "RemoveAsset")
    {
      string id = command->get<string>("assetId");
      auto device = command->maybeGet<string>("device");
      DevicePtr dev {nullptr};
      if (device)
        dev = m_agent->findDeviceByUUIDorName(*device);
      m_agent->removeAsset(dev, id);
    }
    else if (cmd == "RemoveAll")
    {
      auto type = command->maybeGet<string>("type");
      auto device = command->maybeGet<string>("device");
      asset::AssetList list;
      m_agent->removeAllAssets(device, type, nullopt, list);
    }
    else
    {
      LOG(error) << "Invalid assent command: " << cmd;
    }
  }

  void Agent::updateAssetCounts(const DevicePtr &device, const std::optional<std::string> type)
  {
    if (!device)
      return;

    auto dc = device->getAssetCount();
    if (dc)
    {
      if (type)
      {
        auto count = m_assetStorage->getCountForDeviceAndType(*device->getUuid(), *type);

        DataSet set;
        if (count > 0)
          set.emplace(*type, int64_t(count));
        else
          set.emplace(*type, DataSetValue(), true);

        m_loopback->receive(dc, {{"VALUE", set}});
      }
      else
      {
        auto counts = m_assetStorage->getCountsByTypeForDevice(*device->getUuid());

        DataSet set;

        for (auto &[t, count] : counts)
        {
          if (count > 0)
            set.emplace(t, int64_t(count));
          else
            set.emplace(t, DataSetValue(), true);
        }

        m_loopback->receive(dc, {{"resetTriggered", "RESET_COUNTS"s}, {"VALUE", set}});
      }
    }
  }

  void Agent::receiveCommand(const std::string &deviceName, const std::string &command,
                             const std::string &value, const std::string &source)
  {
    DevicePtr device {nullptr};
    device = findDeviceByUUIDorName(deviceName);

    std::string oldName, oldUuid;
    if (device)
    {
      oldName = *device->getComponentName();
      oldUuid = *device->getUuid();
    }

    static std::unordered_map<string, function<void(DevicePtr, const string &value)>>
        deviceCommands {
            {"manufacturer", mem_fn(&Device::setManufacturer)},
            {"station", mem_fn(&Device::setStation)},
            {"serialnumber", mem_fn(&Device::setSerialNumber)},
            {"description", mem_fn(&Device::setDescriptionValue)},
            {"nativename",
             [](DevicePtr device, const string &name) { device->setProperty("nativeName", name); }},
            {"calibration",
             [](DevicePtr device, const string &value) {
               istringstream line(value);

               // Look for name|factor|offset triples
               string name, factor, offset;
               while (getline(line, name, '|') && getline(line, factor, '|') &&
                      getline(line, offset, '|'))
               {
                 // Convert to a floating point number
                 auto di = device->getDeviceDataItem(name);
                 if (!di)
                   LOG(warning) << "Cannot find data item to calibrate for " << name;
                 else
                 {
                   double fact_value = stod(factor);
                   double off_value = stod(offset);

                   device_model::data_item::UnitConversion conv(fact_value, off_value);
                   di->setConverter(conv);
                 }
               }
             }},
        };

    static std::unordered_map<string, string> adapterDataItems {
        {"adapterversion", "_adapter_software_version"},
        {"mtconnectversion", "_mtconnect_version"},
    };

    if (command == "uuid")
    {
      if (!device->preserveUuid())
      {
        auto &idx = m_deviceIndex.get<ByUuid>();
        auto it = idx.find(oldUuid);
        if (it != idx.end())
        {
          idx.modify(it, [&value](DevicePtr &ptr) { ptr->setUuid(value); });
        }
        deviceChanged(device, oldUuid, oldName);
      }
    }
    else
    {
      auto action = deviceCommands.find(command);
      if (action == deviceCommands.end())
      {
        auto agentDi = adapterDataItems.find(command);
        if (agentDi == adapterDataItems.end())
        {
          LOG(warning) << "Unknown command '" << command << "' for device '" << deviceName;
        }
        else
        {
          auto id = source + agentDi->second;
          auto di = getDataItemForDevice("Agent", id);
          if (di)
            m_loopback->receive(di, value);
          else
          {
            LOG(warning) << "Cannot find data item for the Agent device when processing command "
                         << command << " with value " << value << " for adapter " << source;
          }
        }
      }
      else
      {
        action->second(device, value);
        deviceChanged(device, oldUuid, oldName);
      }
    }
  }

  // -------------------------------------------
  // End
  // -------------------------------------------
}  // namespace mtconnect
