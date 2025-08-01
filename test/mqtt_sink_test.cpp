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
// Ensure that gtest is the first header otherwise Windows raises an error
#include <gtest/gtest.h>
// Keep this comment to keep gtest.h above. (clang-format off/on is not working here!)

#include <string>

#include <nlohmann/json.hpp>

#include "agent_test_helper.hpp"
#include "buffer/checkpoint.hpp"
#include "device_model/data_item/data_item.hpp"
#include "entity/entity.hpp"
#include "entity/json_parser.hpp"
#include "mqtt/mqtt_client_impl.hpp"
#include "mqtt/mqtt_server_impl.hpp"
#include "printer/json_printer.hpp"
#include "sink/mqtt_sink/mqtt_service.hpp"

using namespace std;
using namespace mtconnect;
using namespace mtconnect::device_model::data_item;
using namespace mtconnect::sink::mqtt_sink;
using namespace mtconnect::asset;
using namespace mtconnect::configuration;

using json = nlohmann::json;

class MqttSinkTest : public testing::Test
{
protected:
  void SetUp() override
  {
    m_agentTestHelper = make_unique<AgentTestHelper>();
    m_jsonPrinter = std::make_unique<printer::JsonPrinter>(2, true);
  }

  void TearDown() override
  {
    const auto agent = m_agentTestHelper->getAgent();
    if (agent)
    {
      m_agentTestHelper->getAgent()->stop();
      m_agentTestHelper->m_ioContext.run_for(100ms);
    }
    if (m_client)
    {
      m_client->stop();
      m_agentTestHelper->m_ioContext.run_for(100ms);
      m_client.reset();
    }
    if (m_server)
    {
      m_server->stop();
      m_agentTestHelper->m_ioContext.run_for(500ms);
      m_server.reset();
    }
    m_agentTestHelper.reset();
    m_jsonPrinter.reset();
  }

  void createAgent(std::string testFile = {}, ConfigOptions options = {})
  {
    if (testFile == "")
      testFile = "/samples/test_config.xml";

    ConfigOptions opts(options);
    MergeOptions(opts, {{"MqttSink", true},
                        {configuration::MqttPort, m_port},
                        {configuration::MqttHost, "127.0.0.1"s}});
    m_agentTestHelper->createAgent("/samples/test_config.xml", 8, 4, "2.0", 25, false, true, opts);
    addAdapter();

    m_agentTestHelper->getAgent()->start();
  }

  void createServer(const ConfigOptions &options)
  {
    using namespace mtconnect::configuration;
    ConfigOptions opts(options);
    MergeOptions(opts, {{ServerIp, "127.0.0.1"s},
                        {MqttPort, 0},
                        {MqttTls, false},
                        {AutoAvailable, false},
                        {RealTime, false}});

    m_server =
        make_shared<mtconnect::mqtt_server::MqttTcpServer>(m_agentTestHelper->m_ioContext, opts);
  }

  template <typename Rep, typename Period>
  bool waitFor(const chrono::duration<Rep, Period> &time, function<bool()> pred)
  {
    boost::asio::steady_timer timer(m_agentTestHelper->m_ioContext);
    timer.expires_from_now(time);
    bool timeout = false;
    timer.async_wait([&timeout](boost::system::error_code ec) {
      if (!ec)
      {
        timeout = true;
      }
    });

    while (!timeout && !pred())
    {
      m_agentTestHelper->m_ioContext.run_for(100ms);
    }
    timer.cancel();

    return pred();
  }

  void startServer()
  {
    if (m_server)
    {
      bool start = m_server->start();
      if (start)
      {
        m_port = m_server->getPort();
        m_agentTestHelper->m_ioContext.run_for(500ms);
      }
    }
  }

  void createClient(const ConfigOptions &options, unique_ptr<ClientHandler> &&handler)
  {
    ConfigOptions opts(options);
    MergeOptions(opts, {{MqttHost, "127.0.0.1"s},
                        {MqttPort, m_port},
                        {MqttTls, false},
                        {AutoAvailable, false},
                        {RealTime, false}});
    m_client = make_shared<mtconnect::mqtt_client::MqttTcpClient>(m_agentTestHelper->m_ioContext,
                                                                  opts, move(handler));
  }

  bool startClient()
  {
    bool started = m_client && m_client->start();
    if (started)
    {
      return waitFor(1s, [this]() { return m_client->isConnected(); });
    }
    return started;
  }

  void addAdapter(ConfigOptions options = ConfigOptions {})
  {
    m_agentTestHelper->addAdapter(options, "localhost", 7878,
                                  m_agentTestHelper->m_agent->defaultDevice()->getName());
  }

  std::unique_ptr<printer::JsonPrinter> m_jsonPrinter;
  std::shared_ptr<mtconnect::mqtt_server::MqttServer> m_server;
  std::shared_ptr<MqttClient> m_client;
  std::shared_ptr<MqttService> m_service;
  std::unique_ptr<AgentTestHelper> m_agentTestHelper;
  uint16_t m_port {0};
};

TEST_F(MqttSinkTest, mqtt_sink_should_be_loaded_by_agent)
{
  createAgent();
  auto service = m_agentTestHelper->getMqttService();

  ASSERT_TRUE(service);
}

TEST_F(MqttSinkTest, mqtt_sink_should_connect_to_broker)
{
  ConfigOptions options;
  createServer(options);
  startServer();

  ASSERT_NE(0, m_port);

  createAgent();
  auto service = m_agentTestHelper->getMqttService();

  ASSERT_TRUE(waitFor(1s, [&service]() { return service->isConnected(); }));
}

TEST_F(MqttSinkTest, mqtt_sink_should_connect_to_broker_with_UserNameandPassword)
{
  ConfigOptions options {{MqttUserName, "MQTT-SINK"s}, {MqttPassword, "mtconnect"s}};
  createServer(options);
  startServer();

  ASSERT_NE(0, m_port);

  createAgent("", options);
  auto service = m_agentTestHelper->getMqttService();

  ASSERT_TRUE(waitFor(1s, [&service]() { return service->isConnected(); }));
}

TEST_F(MqttSinkTest, mqtt_sink_should_connect_to_broker_without_UserNameandPassword)
{
  ConfigOptions options;
  createServer(options);
  startServer();

  ASSERT_NE(0, m_port);

  createAgent();
  auto service = m_agentTestHelper->getMqttService();

  ASSERT_TRUE(waitFor(1s, [&service]() { return service->isConnected(); }));
}

TEST_F(MqttSinkTest, mqtt_sink_should_publish_device)
{
  ConfigOptions options;
  createServer(options);
  startServer();
  ASSERT_NE(0, m_port);

  entity::JsonParser parser;

  auto handler = make_unique<ClientHandler>();
  bool gotDevice = false;
  handler->m_receive = [&gotDevice, &parser](std::shared_ptr<MqttClient> client,
                                             const std::string &topic, const std::string &payload) {
    EXPECT_EQ("MTConnect/Device/000", topic);

    ErrorList list;
    auto ptr = parser.parse(device_model::Device::getRoot(), payload, "2.0", list);
    EXPECT_EQ(0, list.size());
    auto dev = dynamic_pointer_cast<device_model::Device>(ptr);
    EXPECT_TRUE(dev);
    EXPECT_EQ("LinuxCNC", dev->getComponentName());
    EXPECT_EQ("000", *dev->getUuid());

    gotDevice = true;
  };

  createClient(options, move(handler));
  ASSERT_TRUE(startClient());
  m_client->subscribe("MTConnect/Device/000");

  createAgent();

  auto service = m_agentTestHelper->getMqttService();

  ASSERT_TRUE(waitFor(1s, [&service]() { return service->isConnected(); }));

  waitFor(2s, [&gotDevice]() { return gotDevice; });
}

TEST_F(MqttSinkTest, mqtt_sink_should_publish_Streams)
{
  ConfigOptions options;
  createServer(options);
  startServer();
  ASSERT_NE(0, m_port);

  entity::JsonParser parser;

  auto handler = make_unique<ClientHandler>();
  bool foundLineDataItem = false;
  handler->m_receive = [&foundLineDataItem](std::shared_ptr<MqttClient> client,
                                            const std::string &topic, const std::string &payload) {
    EXPECT_EQ("MTConnect/Observation/000/Controller[Controller]/Path/Line[line]", topic);

    auto jdoc = json::parse(payload);
    string value = jdoc.at("/value"_json_pointer).get<string>();
    if (value == string("204"))
    {
      EXPECT_TRUE(true);
      foundLineDataItem = true;
    }
  };
  createClient(options, move(handler));
  ASSERT_TRUE(startClient());

  createAgent();
  auto service = m_agentTestHelper->getMqttService();
  ASSERT_TRUE(waitFor(1s, [&service]() { return service->isConnected(); }));

  m_agentTestHelper->m_adapter->processData("2021-02-01T12:00:00Z|line|204");

  m_client->subscribe("MTConnect/Observation/000/Controller[Controller]/Path/Line[line]");

  waitFor(2s, [&foundLineDataItem]() { return foundLineDataItem; });
}

TEST_F(MqttSinkTest, mqtt_sink_should_publish_Asset)
{
  ConfigOptions options;
  createServer(options);
  startServer();
  ASSERT_NE(0, m_port);

  entity::JsonParser parser;

  auto handler = make_unique<ClientHandler>();
  bool gotControllerDataItem = false;
  handler->m_receive = [&gotControllerDataItem](std::shared_ptr<MqttClient>,
                                                const std::string &topic,
                                                const std::string &payload) {
    EXPECT_EQ("MTConnect/Asset/0001", topic);
    auto jdoc = json::parse(payload);
    string id = jdoc.at("/Part/assetId"_json_pointer).get<string>();
    if (id == string("0001"))
    {
      EXPECT_TRUE(true);
      gotControllerDataItem = true;
    }
  };
  createClient(options, move(handler));
  ASSERT_TRUE(startClient());

  createAgent();
  auto service = m_agentTestHelper->getMqttService();
  ASSERT_TRUE(waitFor(1s, [&service]() { return service->isConnected(); }));

  m_agentTestHelper->m_adapter->processData(
      "2021-02-01T12:00:00Z|@ASSET@|@1|Part|<Part assetId='1'>TEST 1</Part>");

  m_client->subscribe("MTConnect/Asset/0001");

  waitFor(3s, [&gotControllerDataItem]() { return gotControllerDataItem; });
}

TEST_F(MqttSinkTest, mqtt_sink_should_publish_RotaryMode)
{
  ConfigOptions options;
  createServer(options);
  startServer();
  ASSERT_NE(0, m_port);

  entity::JsonParser parser;

  auto handler = make_unique<ClientHandler>();
  bool gotRotaryMode = false;
  handler->m_receive = [&gotRotaryMode](std::shared_ptr<MqttClient>, const std::string &topic,
                                        const std::string &payload) {
    EXPECT_EQ("MTConnect/Observation/000/Axes[Axes]/Rotary[C]/Events/RotaryMode[Smode]", topic);
    auto jdoc = json::parse(payload);

    string id = jdoc.at("/value"_json_pointer).get<string>();
    if (id == string("SPINDLE"))
    {
      EXPECT_TRUE(true);
      gotRotaryMode = true;
    }
  };

  createClient(options, move(handler));
  ASSERT_TRUE(startClient());

  createAgent("/samples/discrete_example.xml");
  auto service = m_agentTestHelper->getMqttService();
  ASSERT_TRUE(waitFor(1s, [&service]() { return service->isConnected(); }));

  m_agentTestHelper->m_adapter->processData(
      "2021-02-01T12:00:00Z|block|G01X00|Smode|INDEX|line|204");

  m_client->subscribe("MTConnect/Observation/000/Axes[Axes]/Rotary[C]/Events/RotaryMode[Smode]");

  waitFor(3s, [&gotRotaryMode]() { return gotRotaryMode; });
}

TEST_F(MqttSinkTest, mqtt_sink_should_publish_Dataset)
{
  ConfigOptions options;
  createServer(options);
  startServer();
  ASSERT_NE(0, m_port);
  entity::JsonParser parser;
  auto handler = make_unique<ClientHandler>();
  bool gotControllerDataItem = false;
  handler->m_receive = [&gotControllerDataItem](std::shared_ptr<MqttClient>,
                                                const std::string &topic,
                                                const std::string &payload) {
    EXPECT_EQ("MTConnect/Observation/000/Controller[Controller]/Path/VARIABLE[vars]", topic);
    auto jdoc = json::parse(payload);
    string id = jdoc.at("/Part/a"_json_pointer).get<string>();

    if (id == string("1"))
    {
      EXPECT_TRUE(true);
      gotControllerDataItem = true;
    }
  };
  createClient(options, move(handler));
  ASSERT_TRUE(startClient());
  createAgent("/samples/data_set.xml");
  auto service = m_agentTestHelper->getMqttService();
  ASSERT_TRUE(waitFor(1s, [&service]() { return service->isConnected(); }));
  m_agentTestHelper->m_adapter->processData("TIME|vars|a=1 b=2 c=3");
  m_client->subscribe("MTConnect/Observation/000/Controller[Controller]/Path/VARIABLE[vars]");
  waitFor(3s, [&gotControllerDataItem]() { return gotControllerDataItem; });
}

TEST_F(MqttSinkTest, mqtt_sink_should_publish_Table)
{
  ConfigOptions options;
  createServer(options);
  startServer();
  ASSERT_NE(0, m_port);
  entity::JsonParser parser;
  auto handler = make_unique<ClientHandler>();
  bool gotControllerDataItem = false;
  handler->m_receive = [&gotControllerDataItem](std::shared_ptr<MqttClient>,
                                                const std::string &topic,
                                                const std::string &payload) {
    EXPECT_EQ(
        "MTConnect/Observation/000/Controller[Controller]/Path[path]/Events/WorkOffsetTable[wpo]",
        topic);
    auto jdoc = json::parse(payload);

    auto jValue = jdoc.at("/value"_json_pointer);
    int count = 0;
    if (jValue.is_object())
    {
      for (auto &[key, value] : jValue.items())
      {
        if (key == "G53.1" || key == "G53.2" || key == "G53.3")
        {
          for (auto &[subKey, subValue] : value.items())
          {
            if (key == "G53.1" && ((subKey == "X" && subValue.get<float>() == 1) ||
                                   (subKey == "Y" && subValue.get<float>() == 2) ||
                                   (subKey == "Z" && subValue.get<float>() == 3)))
            {
              count++;
            }
            else if (key == "G53.2" && ((subKey == "X" && subValue.get<float>() == 4) ||
                                        (subKey == "Y" && subValue.get<float>() == 5) ||
                                        (subKey == "Z" && subValue.get<float>() == 6)))
            {
              count++;
            }
            else if (key == "G53.3" && ((subKey == "X" && subValue.get<float>() == 7.0) ||
                                        (subKey == "Y" && subValue.get<float>() == 8.0) ||
                                        (subKey == "Z" && subValue.get<float>() == 9.0) ||
                                        (subKey == "U" && subValue.get<float>() == 10.0)))
            {
              count++;
            }
          }
        }
      }
      if (count == 10)
      {
        EXPECT_TRUE(true);
        gotControllerDataItem = true;
      }
    }
  };
  createClient(options, move(handler));
  ASSERT_TRUE(startClient());
  createAgent("/samples/data_set.xml");
  auto service = m_agentTestHelper->getMqttService();
  ASSERT_TRUE(waitFor(1s, [&service]() { return service->isConnected(); }));

  m_agentTestHelper->m_adapter->processData(
      "2021-02-01T12:00:00Z|wpo|G53.1={X=1.0 Y=2.0 Z=3.0} G53.2={X=4.0 Y=5.0 Z=6.0}"
      "G53.3={X=7.0 Y=8.0 Z=9 U=10.0}");

  m_client->subscribe(
      "MTConnect/Observation/000/Controller[Controller]/Path[path]/Events/"
      "WorkOffsetTable[wpo]");

  waitFor(3s, [&gotControllerDataItem]() { return gotControllerDataItem; });
}

TEST_F(MqttSinkTest, mqtt_sink_should_publish_Temperature)
{
  ConfigOptions options;
  createServer(options);
  startServer();
  ASSERT_NE(0, m_port);

  entity::JsonParser parser;

  auto handler = make_unique<ClientHandler>();
  bool gotTemperature = false;
  handler->m_receive = [&gotTemperature](std::shared_ptr<MqttClient>, const std::string &topic,
                                         const std::string &payload) {
    EXPECT_EQ(
        "MTConnect/Observation/000/Axes[Axes]/Linear[Z]/Motor[motor_name]/Samples/"
        "Temperature[z_motor_temp]",
        topic);
    auto jdoc = json::parse(payload);

    auto value = jdoc.at("/value"_json_pointer);
    double load = 81.0;
    if (value.is_number())
    {
      if (load == double(value))
      {
        EXPECT_TRUE(true);
        gotTemperature = true;
      }
    }
  };

  createClient(options, move(handler));
  ASSERT_TRUE(startClient());

  createAgent();
  auto service = m_agentTestHelper->getMqttService();
  ASSERT_TRUE(waitFor(1s, [&service]() { return service->isConnected(); }));

  m_agentTestHelper->m_adapter->processData("2018-04-27T05:00:26.555666|z_motor_temp|81");

  m_client->subscribe(
      "MTConnect/Observation/000/Axes[Axes]/Linear[Z]/Motor[motor_name]/Samples/"
      "Temperature[z_motor_temp]");

  waitFor(3s, [&gotTemperature]() { return gotTemperature; });
}

TEST_F(MqttSinkTest, mqtt_sink_should_publish_LinearLoad)
{
  ConfigOptions options;
  createServer(options);
  startServer();
  ASSERT_NE(0, m_port);
  entity::JsonParser parser;
  auto handler = make_unique<ClientHandler>();
  bool gotLinearLoad = false;
  handler->m_receive = [&gotLinearLoad](std::shared_ptr<MqttClient>, const std::string &topic,
                                        const std::string &payload) {
    EXPECT_EQ("MTConnect/Observation/000/Axes[Axes]/Linear[X]/Load[Xload]", topic);
    auto jdoc = json::parse(payload);
    auto value = jdoc.at("/value"_json_pointer);
    double load = 50.0;
    if (value.is_number())
    {
      if (load == double(value))
      {
        EXPECT_TRUE(true);
        gotLinearLoad = true;
      }
    }
  };
  createClient(options, move(handler));
  ASSERT_TRUE(startClient());
  createAgent();
  auto service = m_agentTestHelper->getMqttService();
  ASSERT_TRUE(waitFor(1s, [&service]() { return service->isConnected(); }));
  m_agentTestHelper->m_adapter->processData("2018-04-27T05:00:26.555666|Xload|50");
  m_client->subscribe("MTConnect/Observation/000/Axes[Axes]/Linear[X]/Load[Xload]");
  waitFor(3s, [&gotLinearLoad]() { return gotLinearLoad; });
}

TEST_F(MqttSinkTest, mqtt_sink_should_publish_DynamicCalibration)
{
  ConfigOptions options;
  createServer(options);
  startServer();
  ASSERT_NE(0, m_port);

  entity::JsonParser parser;

  auto handler = make_unique<ClientHandler>();
  bool gotCalibration = false;
  handler->m_receive = [&gotCalibration](std::shared_ptr<MqttClient>, const std::string &topic,
                                         const std::string &payload) {
    EXPECT_EQ(
        "MTConnect/Observation/000/Axes[Axes]/Linear[X]/Samples/PositionTimeSeries.Actual[Xts]",
        topic);
    auto jdoc = json::parse(payload);

    auto value = jdoc.at("/value"_json_pointer);

    if (value.is_array())
    {
      if (value.size() == 25)
      {
        EXPECT_TRUE(true);
        gotCalibration = true;
      }
    }
  };

  createClient(options, move(handler));
  ASSERT_TRUE(startClient());

  createAgent();
  auto service = m_agentTestHelper->getMqttService();
  ASSERT_TRUE(waitFor(1s, [&service]() { return service->isConnected(); }));

  m_agentTestHelper->m_adapter->processData(
      "2021-02-01T12:00:00Z|Xts|25|| 5118 5118 5118 5118 5118 5118 5118 5118 5118 5118 5118 5118 "
      "5119 5119 5118 "
      "5118 5117 5117 5119 5119 5118 5118 5118 5118 5118");

  m_client->subscribe(
      "MTConnect/Observation/000/Axes[Axes]/Linear[X]/Samples/PositionTimeSeries.Actual[Xts]");

  waitFor(3s, [&gotCalibration]() { return gotCalibration; });
}
