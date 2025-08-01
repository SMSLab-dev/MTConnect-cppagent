//
// Copyright Copyright 2009-2022, AMT � The Association For Manufacturing Technology (�AMT�)
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

#include "composition.hpp"

#include <sstream>

#include "configuration/configuration.hpp"
#include "description.hpp"
#include "entity/entity.hpp"

using namespace std;

namespace mtconnect {
  using namespace entity;
  using namespace device_model::configuration;

  namespace device_model {
    FactoryPtr Composition::getFactory()
    {
      static FactoryPtr compositions;
      if (!compositions)
      {
        auto config = Configuration::getFactory()->deepCopy();
        auto composition = make_shared<Factory>(
            Requirements {Requirement("id", true), Requirement("uuid", false),
                          Requirement("name", false), Requirement("type", true),
                          Requirement("Description", ENTITY, Description::getFactory(), false),
                          Requirement("Configuration", ENTITY, config, false)},
            [](const std::string &name, Properties &props) -> EntityPtr {
              auto ptr = make_shared<Composition>(name, props);
              return dynamic_pointer_cast<Entity>(ptr);
            });

        compositions = make_shared<Factory>(Requirements {
            Requirement("Composition", ENTITY, composition, 1, Requirement::Infinite)});
      }
      return compositions;
    }

    FactoryPtr Composition::getRoot()
    {
      static auto root = make_shared<Factory>(Requirements {
          Requirement("Compositions", ENTITY_LIST, Composition::getFactory(), false)});

      return root;
    }
  }  // namespace device_model
}  // namespace mtconnect
