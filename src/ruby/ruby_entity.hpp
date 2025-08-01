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

#pragma once

#include <mruby-time/include/mruby/time.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/value.h>

#include "device_model/data_item/data_item.hpp"
#include "device_model/device.hpp"
#include "entity/data_set.hpp"
#include "entity/entity.hpp"
#include "pipeline/shdr_tokenizer.hpp"
#include "pipeline/timestamp_extractor.hpp"
#include "ruby_smart_ptr.hpp"
#include "ruby_type.hpp"

namespace mtconnect::ruby {
  using namespace mtconnect;
  using namespace device_model;
  using namespace data_item;
  using namespace entity;
  using namespace std;

  inline mrb_value toRuby(mrb_state *mrb, const DataSet &value);
  inline mrb_value toRuby(mrb_state *mrb, const DataSetValue &value)
  {
    mrb_value rv;

    rv = visit(overloaded {[](const std::monostate &v) -> mrb_value { return mrb_nil_value(); },
                           [mrb](const std::string &v) -> mrb_value {
                             return mrb_str_new_cstr(mrb, v.c_str());
                           },
                           [mrb](const entity::DataSet &v) -> mrb_value { return toRuby(mrb, v); },
                           [mrb](const int64_t v) -> mrb_value { return mrb_int_value(mrb, v); },
                           [mrb](const double v) -> mrb_value { return mrb_float_value(mrb, v); }},
               value);

    return rv;
  }

  inline mrb_value toRuby(mrb_state *mrb, const DataSet &set)
  {
    mrb_value hash = mrb_hash_new(mrb);
    for (const auto &entry : set)
    {
      auto value = (entry.m_value);

      mrb_sym k = mrb_intern_cstr(mrb, entry.m_key.c_str());
      mrb_value v = toRuby(mrb, value);

      mrb_hash_set(mrb, hash, mrb_symbol_value(k), v);
    }

    return hash;
  }

  inline void dataSetFromRuby(mrb_state *mrb, mrb_value value, DataSet &dataSet);
  inline bool dataSetValueFromRuby(mrb_state *mrb, mrb_value value, DataSetValue &dsv)
  {
    bool res = true;
    switch (mrb_type(value))
    {
      case MRB_TT_SYMBOL:
      case MRB_TT_STRING:
        dsv.emplace<string>(stringFromRuby(mrb, value));
        break;

      case MRB_TT_FIXNUM:
        dsv.emplace<int64_t>(mrb_fixnum(value));
        break;

      case MRB_TT_FLOAT:
        dsv.emplace<double>(mrb_to_flo(mrb, value));
        break;

      case MRB_TT_HASH:
      {
        DataSet inner;
        dataSetFromRuby(mrb, value, inner);
        dsv.emplace<entity::DataSet>(inner);
        break;
      }

      default:
      {
        LOG(warning) << "DataSet cannot conver type: "
                     << stringFromRuby(mrb, mrb_inspect(mrb, value));
        res = false;
        break;
      }
    }
    return res;
  }

  inline void dataSetFromRuby(mrb_state *mrb, mrb_value value, DataSet &dataSet)
  {
    auto hash = mrb_hash_ptr(value);
    mrb_hash_foreach(
        mrb, hash,
        [](mrb_state *mrb, mrb_value key, mrb_value val, void *data) {
          DataSet *dataSet = static_cast<DataSet *>(data);
          string k = stringFromRuby(mrb, key);
          DataSetValue dsv;
          if (dataSetValueFromRuby(mrb, val, dsv))
            dataSet->emplace(k, dsv);

          return 0;
        },
        &dataSet);
  }

  inline Value valueFromRuby(mrb_state *mrb, mrb_value value)
  {
    Value res;

    if (mrb_nil_p(value))
    {
      res.emplace<std::nullptr_t>();
      return res;
    }

    switch (mrb_type(value))
    {
      case MRB_TT_UNDEF:
        res.emplace<std::monostate>();
        break;

      case MRB_TT_STRING:
        res.emplace<string>(mrb_str_to_cstr(mrb, value));
        break;

      case MRB_TT_FIXNUM:
        res.emplace<int64_t>(mrb_fixnum(value));
        break;

      case MRB_TT_FLOAT:
        res.emplace<double>(mrb_to_flo(mrb, value));
        break;

      case MRB_TT_TRUE:
        res.emplace<bool>(true);
        break;

      case MRB_TT_FALSE:
        res.emplace<bool>(false);
        break;

      case MRB_TT_HASH:
      {
        DataSet ds;
        dataSetFromRuby(mrb, value, ds);
        res.emplace<DataSet>(ds);
        break;
      }

      case MRB_TT_ARRAY:
      {
        auto ary = mrb_ary_ptr(value);
        auto size = ARY_LEN(ary);
        auto values = ARY_PTR(ary);

        if (mrb_type(values[0]) == MRB_TT_FIXNUM || mrb_type(values[0]) == MRB_TT_FLOAT)
        {
          res.emplace<Vector>();
          Vector &out = get<Vector>(res);

          for (int i = 0; i < size; i++)
          {
            mrb_value &v = values[i];
            auto t = mrb_type(v);
            if (t == MRB_TT_FIXNUM)
              out.emplace_back((double)mrb_integer(v));
            else if (t == MRB_TT_FLOAT)
              out.emplace_back(mrb_float(v));
            else
            {
              auto in = mrb_inspect(mrb, value);
              LOG(warning) << "Invalid type for array: " << mrb_str_to_cstr(mrb, in);
            }
          }
        }
        else
        {
          auto mod = mrb_module_get(mrb, "MTConnect");
          auto klass = mrb_class_get_under(mrb, mod, "Entity");

          res.emplace<EntityList>();
          EntityList &list = get<EntityList>(res);
          for (int i = 0; i < size; i++)
          {
            mrb_value &v = values[i];
            if (mrb_type(v) == MRB_TT_DATA)
            {
              if (mrb_obj_is_kind_of(mrb, value, klass))
              {
                auto ent = MRubySharedPtr<Entity>::unwrap(mrb, value);
                list.emplace_back(ent);
              }
            }
          }
        }
        break;
      }

      case MRB_TT_DATA:
      case MRB_TT_OBJECT:
      {
        string kn(mrb_obj_classname(mrb, value));
        // Convert time
        if (kn == "Time")
        {
          res.emplace<Timestamp>(timestampFromRuby(mrb, value));
        }
        else
        {
          auto mod = mrb_module_get(mrb, "MTConnect");
          auto klass = mrb_class_get_under(mrb, mod, "Entity");
          if (mrb_obj_is_kind_of(mrb, value, klass))
          {
            auto ent = MRubySharedPtr<Entity>::unwrap(mrb, value);
            res.emplace<EntityPtr>(ent);
          }
        }
        break;
      }

      default:
      {
        auto in = mrb_inspect(mrb, value);
        LOG(warning) << "Unhandled type for Value: " << mrb_str_to_cstr(mrb, in);
        res.emplace<std::monostate>();
        break;
      }
    }

    return res;
  }

  inline mrb_value toRuby(mrb_state *mrb, const Value &value)
  {
    mrb_value res = visit(
        overloaded {
            [](const std::monostate &) -> mrb_value { return mrb_nil_value(); },
            [](const std::nullptr_t &) -> mrb_value { return mrb_nil_value(); },

            // Not handled yet
            [mrb](const EntityPtr &entity) -> mrb_value {
              return MRubySharedPtr<Entity>::wrap(mrb, "Entity", entity);
            },
            [mrb](const EntityList &list) -> mrb_value {
              mrb_value ary = mrb_ary_new_capa(mrb, list.size());

              for (auto &e : list)
                mrb_ary_push(mrb, ary, MRubySharedPtr<Entity>::wrap(mrb, "Entity", e));

              return ary;
            },
            [mrb](const entity::DataSet &v) -> mrb_value { return toRuby(mrb, v); },

            // Handled types
            [mrb](const entity::Vector &v) -> mrb_value {
              mrb_value ary = mrb_ary_new_capa(mrb, v.size());
              for (auto &f : v)
                mrb_ary_push(mrb, ary, mrb_float_value(mrb, f));
              return ary;
            },
            [mrb](const Timestamp &v) -> mrb_value { return toRuby(mrb, v); },
            [mrb](const string &arg) -> mrb_value { return mrb_str_new_cstr(mrb, arg.c_str()); },
            [](const bool arg) -> mrb_value { return mrb_bool_value(static_cast<mrb_bool>(arg)); },
            [mrb](const double arg) -> mrb_value { return mrb_float_value(mrb, arg); },
            [mrb](const int64_t arg) -> mrb_value { return mrb_int_value(mrb, arg); }},
        value);

    return res;
  }

  inline bool fromRuby(mrb_state *mrb, mrb_value value, Properties &props)
  {
    if (mrb_type(value) != MRB_TT_HASH)
    {
      Value v = valueFromRuby(mrb, value);
      props.emplace("VALUE", v);
    }
    else
    {
      auto hash = mrb_hash_ptr(value);
      mrb_hash_foreach(
          mrb, hash,
          [](mrb_state *mrb, mrb_value key, mrb_value val, void *data) {
            Properties *props = static_cast<Properties *>(data);
            string k = stringFromRuby(mrb, key);
            auto v = valueFromRuby(mrb, val);

            props->emplace(k, v);

            return 0;
          },
          &props);
    }

    return true;
  }

  inline mrb_value toRuby(mrb_state *mrb, const Properties &props)
  {
    mrb_value hash = mrb_hash_new(mrb);
    for (auto &[key, value] : props)
    {
      mrb_sym k = mrb_intern_cstr(mrb, key.c_str());
      mrb_value v = toRuby(mrb, value);

      mrb_hash_set(mrb, hash, mrb_symbol_value(k), v);
    }

    return hash;
  }

  struct RubyEntity
  {
    static void initialize(mrb_state *mrb, RClass *module)
    {
      auto entityClass = mrb_define_class_under(mrb, module, "Entity", mrb->object_class);
      MRB_SET_INSTANCE_TT(entityClass, MRB_TT_DATA);

      mrb_define_method(
          mrb, entityClass, "initialize",
          [](mrb_state *mrb, mrb_value self) {
            const char *name;
            mrb_value properties;
            mrb_get_args(mrb, "zo", &name, &properties);

            Properties props;
            fromRuby(mrb, properties, props);

            auto entity = make_shared<Entity>(name, props);
            MRubySharedPtr<Entity>::replace(mrb, self, entity);

            return self;
          },
          MRB_ARGS_REQ(2));

      mrb_define_method(
          mrb, entityClass, "name",
          [](mrb_state *mrb, mrb_value self) {
            auto entity = MRubySharedPtr<Entity>::unwrap(self);
            return mrb_str_new_cstr(mrb, entity->getName().c_str());
          },
          MRB_ARGS_NONE());
      mrb_define_method(
          mrb, entityClass, "value",
          [](mrb_state *mrb, mrb_value self) {
            auto entity = MRubySharedPtr<Entity>::unwrap(self);
            return toRuby(mrb, entity->getValue());
          },
          MRB_ARGS_NONE());
      mrb_define_method(
          mrb, entityClass, "value=",
          [](mrb_state *mrb, mrb_value self) {
            auto entity = MRubySharedPtr<Entity>::unwrap(self);
            mrb_value value;
            mrb_get_args(mrb, "o", &value);
            entity->setValue(valueFromRuby(mrb, value));
            return value;
          },
          MRB_ARGS_REQ(1));

      mrb_define_method(
          mrb, entityClass, "properties",
          [](mrb_state *mrb, mrb_value self) {
            auto entity = MRubySharedPtr<Entity>::unwrap(self);
            auto props = entity->getProperties();

            return toRuby(mrb, props);
          },
          MRB_ARGS_NONE());
      mrb_define_method(
          mrb, entityClass, "[]",
          [](mrb_state *mrb, mrb_value self) {
            auto entity = MRubySharedPtr<Entity>::unwrap(self);
            const char *key;
            mrb_get_args(mrb, "s", &key);

            auto props = entity->getProperties();
            auto it = props.find(key);
            if (it != props.end())
              return toRuby(mrb, it->second);
            else
              return mrb_nil_value();
          },
          MRB_ARGS_REQ(1));
      mrb_define_method(
          mrb, entityClass, "[]=",
          [](mrb_state *mrb, mrb_value self) {
            auto entity = MRubySharedPtr<Entity>::unwrap(self);
            const char *key;
            mrb_value value;
            mrb_get_args(mrb, "so", &key, &value);

            entity->setProperty(key, valueFromRuby(mrb, value));

            return value;
          },
          MRB_ARGS_REQ(1));

      auto componentClass = mrb_define_class_under(mrb, module, "Component", entityClass);
      MRB_SET_INSTANCE_TT(componentClass, MRB_TT_DATA);

      mrb_define_method(
          mrb, componentClass, "children",
          [](mrb_state *mrb, mrb_value self) {
            auto comp = MRubySharedPtr<Entity>::unwrap<Component>(mrb, self);
            mrb_value ary = mrb_ary_new(mrb);
            const auto &list = comp->getChildren();
            if (list)
            {
              auto mod = mrb_module_get(mrb, "MTConnect");
              auto klass = mrb_class_get_under(mrb, mod, "Component");

              for (const auto &c : *list)
              {
                ComponentPtr cmp = dynamic_pointer_cast<Component>(c);
                if (cmp)
                  mrb_ary_push(mrb, ary, MRubySharedPtr<Entity>::wrap(mrb, klass, cmp));
              }
            }

            return ary;
          },
          MRB_ARGS_NONE());
      mrb_define_method(
          mrb, componentClass, "data_items",
          [](mrb_state *mrb, mrb_value self) {
            auto comp = MRubySharedPtr<Entity>::unwrap<Component>(mrb, self);
            mrb_value ary = mrb_ary_new(mrb);
            const auto &list = comp->getDataItems();
            if (list)
            {
              auto mod = mrb_module_get(mrb, "MTConnect");
              auto klass = mrb_class_get_under(mrb, mod, "DataItem");

              for (const auto &c : *list)
              {
                DataItemPtr di = dynamic_pointer_cast<DataItem>(c);
                if (di)
                  mrb_ary_push(mrb, ary, MRubySharedPtr<Entity>::wrap(mrb, klass, di));
              }
            }

            return ary;
          },
          MRB_ARGS_NONE());

      mrb_define_method(
          mrb, componentClass, "uuid",
          [](mrb_state *mrb, mrb_value self) {
            auto comp = MRubySharedPtr<Entity>::unwrap<Component>(mrb, self);
            auto &uuid = comp->getUuid();
            if (uuid)
              return mrb_str_new_cstr(mrb, uuid->c_str());
            else
              return mrb_nil_value();
          },
          MRB_ARGS_NONE());

      auto deviceClass = mrb_define_class_under(mrb, module, "Device", componentClass);
      MRB_SET_INSTANCE_TT(deviceClass, MRB_TT_DATA);

      mrb_define_method(
          mrb, componentClass, "data_item",
          [](mrb_state *mrb, mrb_value self) {
            auto dev = MRubySharedPtr<Entity>::unwrap<Device>(mrb, self);
            const char *name;
            mrb_get_args(mrb, "z", &name);

            auto di = dev->getDeviceDataItem(name);
            if (di)
              return MRubySharedPtr<Entity>::wrap(mrb, "DataItem", di);
            else
              return mrb_nil_value();
          },
          MRB_ARGS_REQ(1));

      auto dataItemClass = mrb_define_class_under(mrb, module, "DataItem", entityClass);
      MRB_SET_INSTANCE_TT(dataItemClass, MRB_TT_DATA);

      mrb_define_method(
          mrb, dataItemClass, "name",
          [](mrb_state *mrb, mrb_value self) {
            auto di = MRubySharedPtr<Entity>::unwrap<DataItem>(mrb, self);
            if (di->getName())
              return mrb_str_new_cstr(mrb, (*di->getName()).c_str());
            else
              return mrb_nil_value();
          },
          MRB_ARGS_NONE());
      mrb_define_method(
          mrb, dataItemClass, "observation_name",
          [](mrb_state *mrb, mrb_value self) {
            auto di = MRubySharedPtr<Entity>::unwrap<DataItem>(mrb, self);
            return mrb_str_new_cstr(mrb, di->getObservationName().c_str());
          },
          MRB_ARGS_NONE());
      mrb_define_method(
          mrb, dataItemClass, "id",
          [](mrb_state *mrb, mrb_value self) {
            auto di = MRubySharedPtr<Entity>::unwrap<DataItem>(mrb, self);
            return mrb_str_new_cstr(mrb, di->getId().c_str());
          },
          MRB_ARGS_NONE());
      mrb_define_method(
          mrb, dataItemClass, "type",
          [](mrb_state *mrb, mrb_value self) {
            auto di = MRubySharedPtr<Entity>::unwrap<DataItem>(mrb, self);
            return mrb_str_new_cstr(mrb, di->getType().c_str());
          },
          MRB_ARGS_NONE());
      mrb_define_method(
          mrb, dataItemClass, "sub_type",
          [](mrb_state *mrb, mrb_value self) {
            auto di = MRubySharedPtr<Entity>::unwrap<DataItem>(mrb, self);
            return mrb_str_new_cstr(mrb, di->getSubType().c_str());
          },
          MRB_ARGS_NONE());
      mrb_define_method(
          mrb, dataItemClass, "topic",
          [](mrb_state *mrb, mrb_value self) {
            auto di = MRubySharedPtr<Entity>::unwrap<DataItem>(mrb, self);
            return mrb_str_new_cstr(mrb, di->getTopic().c_str());
          },
          MRB_ARGS_NONE());
      mrb_define_method(
          mrb, dataItemClass, "topic=",
          [](mrb_state *mrb, mrb_value self) {
            auto di = MRubySharedPtr<Entity>::unwrap<DataItem>(mrb, self);
            char *val;
            mrb_get_args(mrb, "z", &val);

            di->setTopic(val);

            return mrb_str_new_cstr(mrb, val);
          },
          MRB_ARGS_REQ(1));

      auto tokensClass = mrb_define_class_under(mrb, module, "Tokens", entityClass);
      MRB_SET_INSTANCE_TT(tokensClass, MRB_TT_DATA);
      mrb_define_method(
          mrb, tokensClass, "tokens",
          [](mrb_state *mrb, mrb_value self) {
            auto tokens = MRubySharedPtr<Entity>::unwrap<pipeline::Tokens>(mrb, self);

            mrb_value ary = mrb_ary_new(mrb);
            for (auto &token : tokens->m_tokens)
            {
              mrb_ary_push(mrb, ary, mrb_str_new_cstr(mrb, token.c_str()));
            }
            return ary;
          },
          MRB_ARGS_NONE());

      mrb_define_method(
          mrb, tokensClass, "tokens=",
          [](mrb_state *mrb, mrb_value self) {
            auto tokens = MRubySharedPtr<Entity>::unwrap<pipeline::Tokens>(mrb, self);
            mrb_value ary;
            mrb_get_args(mrb, "A", &ary);
            if (mrb_array_p(ary))
            {
              tokens->m_tokens.clear();
              auto aryp = mrb_ary_ptr(ary);
              for (int i = 0; i < ARY_LEN(aryp); i++)
              {
                auto item = ARY_PTR(aryp)[i];
                tokens->m_tokens.push_back(stringFromRuby(mrb, item));
              }
            }
            return ary;
          },
          MRB_ARGS_REQ(1));

      auto timestampedClass = mrb_define_class_under(mrb, module, "Timestamped", tokensClass);
      MRB_SET_INSTANCE_TT(timestampedClass, MRB_TT_DATA);

      mrb_define_method(
          mrb, tokensClass, "timestamp",
          [](mrb_state *mrb, mrb_value self) {
            auto ts = MRubySharedPtr<Entity>::unwrap<pipeline::Timestamped>(mrb, self);

            return toRuby(mrb, ts->m_timestamp);
          },
          MRB_ARGS_NONE());

      mrb_define_method(
          mrb, tokensClass, "timestamp=",
          [](mrb_state *mrb, mrb_value self) {
            auto ts = MRubySharedPtr<Entity>::unwrap<pipeline::Timestamped>(mrb, self);
            mrb_value val;
            mrb_get_args(mrb, "o", &val);

            ts->m_timestamp = timestampFromRuby(mrb, val);

            return val;
          },
          MRB_ARGS_REQ(1));

      mrb_define_method(
          mrb, tokensClass, "duration",
          [](mrb_state *mrb, mrb_value self) {
            auto ts = MRubySharedPtr<Entity>::unwrap<pipeline::Timestamped>(mrb, self);
            if (ts->m_duration)
              return mrb_float_value(mrb, *(ts->m_duration));
            else
              return mrb_nil_value();
          },
          MRB_ARGS_NONE());

      mrb_define_method(
          mrb, tokensClass, "duration=",
          [](mrb_state *mrb, mrb_value self) {
            auto ts = MRubySharedPtr<Entity>::unwrap<pipeline::Timestamped>(mrb, self);
            mrb_float val;
            mrb_get_args(mrb, "f", &val);

            ts->m_duration = val;

            return mrb_float_value(mrb, val);
          },
          MRB_ARGS_REQ(1));
    }
  };
}  // namespace mtconnect::ruby
