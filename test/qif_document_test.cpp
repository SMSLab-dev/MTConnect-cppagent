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

#include <cstdio>
#include <date/date.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "agent.hpp"
#include "agent_test_helper.hpp"
#include "asset/asset.hpp"
#include "asset/qif_document.hpp"
#include "entity/entity.hpp"
#include "entity/json_printer.hpp"
#include "entity/xml_parser.hpp"
#include "entity/xml_printer.hpp"
#include "json_helper.hpp"
#include "printer/xml_printer.hpp"
#include "printer/xml_printer_helper.hpp"
#include "source/adapter/adapter.hpp"

using json = nlohmann::json;
using namespace std;
using namespace mtconnect;
using namespace mtconnect::entity;
using namespace mtconnect::source::adapter;
using namespace mtconnect::asset;
using namespace mtconnect::printer;

class QIFDocumentTest : public testing::Test
{
protected:
  void SetUp() override
  {  // Create an agent with only 16 slots and 8 data items.
    m_agentTestHelper = make_unique<AgentTestHelper>();
    m_agentTestHelper->createAgent("/samples/test_config.xml", 8, 4, "2.0", 25);
    m_agentId = to_string(getCurrentTimeInSec());
    m_device = m_agentTestHelper->m_agent->getDeviceByName("LinuxCNC");

    // Asset types are registered in the agent.
    m_device = m_agentTestHelper->m_agent->getDeviceByName("LinuxCNC");

    m_writer = make_unique<printer::XmlWriter>(true);
  }

  void TearDown() override
  {
    m_agentTestHelper.reset();
    m_writer.reset();
  }

  void addAdapter() { m_agentTestHelper->addAdapter(); }

  std::string m_agentId;
  DevicePtr m_device {nullptr};

  std::unique_ptr<printer::XmlWriter> m_writer;
  std::unique_ptr<AgentTestHelper> m_agentTestHelper;
};

TEST_F(QIFDocumentTest, minimal_qif_definition)
{
  using namespace date;
  const auto doc = R"DOC(
<QIFDocumentWrapper assetId="30d278e0-c150-013a-c34d-4e7f553bbb76" qifDocumentType="PLAN">
  <QIFDocument xmlns="http://qifstandards.org/xsd/qif2"
     xmlns:q="http://qifstandards.org/xsd/qif2"
     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
     versionQIF="2.0.0"
     xsi:schemaLocation="http://qifstandards.org/xsd/qif2 QIFApplications/QIFDocument.xsd">
        <Version>
          <ThisInstanceQPId>fd43400a-29bf-4ec6-b96c-e2f846eb6ff6</ThisInstanceQPId>
        </Version>
        <Product>
        <PartSet N="1">
          <Part id="1">
            <Name>Widget</Name>
            <QPId>ed43400a-29bf-4ec6-b96c-e2f846eb6f00</QPId>
          </Part>
        </PartSet>
        <RootPart>
          <Id>1</Id>
        </RootPart>
    </Product>
  </QIFDocument>
</QIFDocumentWrapper>
)DOC";

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(Asset::getRoot(), doc, "2.0", errors);
  ASSERT_EQ(0, errors.size());

  auto asset = dynamic_cast<Asset *>(entity.get());
  ASSERT_NE(nullptr, asset);

  ASSERT_EQ("30d278e0-c150-013a-c34d-4e7f553bbb76", asset->getAssetId());
  ASSERT_EQ("PLAN", asset->get<string>("qifDocumentType"));

  ASSERT_FALSE(asset->getTimestamp());
  ASSERT_FALSE(asset->getDeviceUuid());

  auto qif = asset->get<EntityPtr>("QIFDocument");
  ASSERT_NE(nullptr, qif);

  ASSERT_EQ("http://qifstandards.org/xsd/qif2", qif->get<string>("xmlns"));
  ASSERT_EQ("http://qifstandards.org/xsd/qif2 QIFApplications/QIFDocument.xsd",
            qif->get<string>("xsi:schemaLocation"));
  ASSERT_EQ("2.0.0", qif->get<string>("versionQIF"));

  auto version = qif->get<EntityPtr>("Version");
  ASSERT_TRUE(version);

  ASSERT_EQ("fd43400a-29bf-4ec6-b96c-e2f846eb6ff6"s, version->get<string>("ThisInstanceQPId"));

  auto product = qif->get<EntityPtr>("Product");
  ASSERT_TRUE(product);
  auto partSet = product->get<EntityPtr>("PartSet");
  ASSERT_TRUE(partSet);
  ASSERT_EQ("1"s, partSet->get<string>("N"));
  auto part = partSet->get<EntityPtr>("Part");
  ASSERT_EQ("1"s, part->get<string>("id"));
  ASSERT_EQ("Widget"s, part->get<string>("Name"));
  ASSERT_EQ("ed43400a-29bf-4ec6-b96c-e2f846eb6f00"s, part->get<string>("QPId"));
  auto root = product->get<EntityPtr>("RootPart");
  ASSERT_TRUE(root);
  ASSERT_EQ("1"s, root->get<string>("Id"));
}

TEST_F(QIFDocumentTest, test_qif_xml_round_trip)
{
  using namespace date;
  const auto doc =
      R"DOC(<QIFDocumentWrapper assetId="30d278e0-c150-013a-c34d-4e7f553bbb76" qifDocumentType="PLAN">
  <QIFDocument versionQIF="2.0.0" xmlns="http://qifstandards.org/xsd/qif2" xmlns:q="http://qifstandards.org/xsd/qif2" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://qifstandards.org/xsd/qif2 QIFApplications/QIFDocument.xsd">
    <Version>
      <ThisInstanceQPId>fd43400a-29bf-4ec6-b96c-e2f846eb6ff6</ThisInstanceQPId>
    </Version>
    <Product>
      <PartSet N="1">
        <Part id="1">
          <Name>Widget</Name>
          <QPId>ed43400a-29bf-4ec6-b96c-e2f846eb6f00</QPId>
        </Part>
      </PartSet>
      <RootPart>
        <Id>1</Id>
      </RootPart>
    </Product>
  </QIFDocument>
</QIFDocumentWrapper>
)DOC";

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(Asset::getRoot(), doc, "2.0", errors);
  ASSERT_EQ(0, errors.size());

  auto asset = dynamic_cast<Asset *>(entity.get());
  ASSERT_NE(nullptr, asset);

  entity::XmlPrinter printer;
  printer.print(*m_writer, entity, {"x"});
  string content = m_writer->getContent();

  ASSERT_EQ(doc, content);
}

TEST_F(QIFDocumentTest, should_generate_json)
{
  using namespace date;
  const auto doc = R"DOC(
<QIFDocumentWrapper assetId="30d278e0-c150-013a-c34d-4e7f553bbb76" qifDocumentType="PLAN">
  <QIFDocument xmlns="http://qifstandards.org/xsd/qif2"
     xmlns:q="http://qifstandards.org/xsd/qif2"
     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
     versionQIF="2.0.0"
     xsi:schemaLocation="http://qifstandards.org/xsd/qif2 QIFApplications/QIFDocument.xsd">
        <Version>
          <ThisInstanceQPId>fd43400a-29bf-4ec6-b96c-e2f846eb6ff6</ThisInstanceQPId>
        </Version>
        <Product>
        <PartSet N="1">
          <Part id="1">
            <Name>Widget</Name>
            <QPId>ed43400a-29bf-4ec6-b96c-e2f846eb6f00</QPId>
          </Part>
        </PartSet>
        <RootPart>
          <Id>1</Id>
        </RootPart>
    </Product>
  </QIFDocument>
</QIFDocumentWrapper>
)DOC";

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(Asset::getRoot(), doc, "2.0", errors);
  ASSERT_EQ(0, errors.size());

  auto asset = dynamic_cast<Asset *>(entity.get());
  ASSERT_NE(nullptr, asset);

  entity::JsonPrinter jsonPrinter(1);
  auto json = jsonPrinter.print(entity);

  stringstream buffer;
  buffer << std::setw(2);
  buffer << json;
  string res = buffer.str();

  ASSERT_EQ(R"DOC({
  "QIFDocumentWrapper": {
    "QIFDocument": {
      "Product": {
        "PartSet": {
          "N": "1",
          "Part": {
            "Name": "Widget",
            "QPId": "ed43400a-29bf-4ec6-b96c-e2f846eb6f00",
            "id": "1"
          }
        },
        "RootPart": {
          "Id": "1"
        }
      },
      "Version": {
        "ThisInstanceQPId": "fd43400a-29bf-4ec6-b96c-e2f846eb6ff6"
      },
      "versionQIF": "2.0.0",
      "xmlns": "http://qifstandards.org/xsd/qif2",
      "xmlns:q": "http://qifstandards.org/xsd/qif2",
      "xmlns:xsi": "http://www.w3.org/2001/XMLSchema-instance",
      "xsi:schemaLocation": "http://qifstandards.org/xsd/qif2 QIFApplications/QIFDocument.xsd"
    },
    "assetId": "30d278e0-c150-013a-c34d-4e7f553bbb76",
    "qifDocumentType": "PLAN"
  }
})DOC"s,
            res);
}
