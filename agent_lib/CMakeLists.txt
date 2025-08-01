set(AGENT_SOURCES
 # src HEADER_FILE_ONLY
        
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/agent.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/logging.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/utilities.hpp"

# src SOURCE_FILES_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/agent.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/utilities.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/version.cpp"  
        

# src/asset HEADER_FILE_ONLY 

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/asset/asset.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/asset/asset_buffer.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/asset/asset_storage.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/asset/cutting_tool.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/asset/file_asset.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/asset/raw_material.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/asset/qif_document.hpp"

# src/asset SOURCE_FILES_ONLY
  
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/asset/asset.cpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/asset/cutting_tool.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/asset/file_asset.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/asset/raw_material.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/asset/qif_document.cpp"
        
# src/buffer HEADER_FILES_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/buffer/checkpoint.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/buffer/circular_buffer.hpp"

# src/buffer SOURCE_FILES_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/buffer/checkpoint.cpp"

# src/configuration HEADER_FILE_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/configuration/agent_config.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/configuration/async_context.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/configuration/config_options.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/configuration/parser.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/configuration/service.hpp"

# src/configuration SOURCE_FILES_ONLY
  
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/configuration/agent_config.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/configuration/parser.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/configuration/service.cpp"

# src/device_model HEADER_FILE_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/agent_device.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/component.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/composition.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/description.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/device.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/reference.hpp"
  
# src/device_model SOURCE_FILES_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/agent_device.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/component.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/composition.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/description.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/device.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/reference.cpp"
  
# src/device_model/configuration HEADER_FILE_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/configuration/configuration.hpp"  
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/configuration/coordinate_systems.hpp"  
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/configuration/motion.hpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/configuration/relationships.hpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/configuration/sensor_configuration.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/configuration/solid_model.hpp"  
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/configuration/specifications.hpp"       
        
# src/device_model/configuration SOURCE_FILES_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/configuration/configuration.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/configuration/coordinate_systems.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/configuration/motion.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/configuration/relationships.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/configuration/sensor_configuration.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/configuration/solid_model.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/configuration/specifications.cpp"      
        
# src/device_model/data_item HEADER_FILE_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/data_item/constraints.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/data_item/data_item.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/data_item/definition.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/data_item/filter.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/data_item/relationships.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/data_item/source.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/data_item/unit_conversion.hpp"

# src/device_model/data_item SOURCE_FILES_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/data_item/data_item.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/device_model/data_item/unit_conversion.cpp"

# src/entity HEADER_FILE_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/data_set.hpp"        
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/entity.hpp"  
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/factory.hpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/json_parser.hpp"     
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/json_printer.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/qname.hpp"   
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/requirement.hpp"     
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/xml_parser.hpp"      
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/xml_printer.hpp"
  
# src/entity SOURCE_FILES_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/data_set.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/entity.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/factory.cpp"  
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/json_parser.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/json_printer.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/requirement.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/xml_parser.cpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/entity/xml_printer.cpp"

# src/mqtt HEADER_FILE_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/mqtt/mqtt_client.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/mqtt/mqtt_server.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/mqtt/mqtt_client_impl.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/mqtt/mqtt_server_impl.hpp"
  
# src/observation HEADER_FILE_ONLY 
        
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/observation/change_observer.hpp"  
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/observation/observation.hpp"
   
#src/observation SOURCE_FILES_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/observation/change_observer.cpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/observation/observation.cpp"        

# src/parser HEADER_FILE_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/xml_parser.hpp"

# src/parser SOURCE_FILES_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/xml_parser.cpp"

# src/pipeline HEADER_FILE_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/convert_sample.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/deliver.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/delta_filter.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/duplicate_filter.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/guard.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/message_mapper.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/mtconnect_xml_transform.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/period_filter.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/pipeline.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/pipeline_context.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/pipeline_contract.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/response_document.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/shdr_token_mapper.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/shdr_tokenizer.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/timestamp_extractor.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/topic_mapper.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/transform.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/upcase_value.hpp"
   
# src/pipeline SOURCE_FILES_ONLY
   
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/deliver.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/shdr_token_mapper.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/timestamp_extractor.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/pipeline/response_document.cpp"

# src/printer HEADER_FILE_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/printer/json_printer.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/printer/printer.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/printer/xml_helper.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/printer/xml_printer.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/printer/xml_printer_helper.hpp"

# src/printer SOURCE_FILES_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/printer/json_printer.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/printer/xml_printer.cpp"

# src/source HEADER_FILE_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/adapter.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/adapter_pipeline.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/agent_adapter/agent_adapter.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/agent_adapter/http_session.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/agent_adapter/https_session.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/agent_adapter/session.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/agent_adapter/session_impl.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/agent_adapter/url_parser.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/mqtt/mqtt_adapter.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/shdr/connector.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/shdr/shdr_adapter.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/shdr/shdr_pipeline.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/error_code.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/loopback_source.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/source.hpp"

# src/source SOURCE_FILES_ONLY
        
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/adapter_pipeline.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/mqtt/mqtt_adapter.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/shdr/connector.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/shdr/shdr_adapter.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/shdr/shdr_pipeline.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/loopback_source.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/source.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/agent_adapter/agent_adapter.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/agent_adapter/url_parser.cpp"
		
# src/sink HEADER_FILE_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/sink.hpp"

# src/sink SOURCE_FILE_ONLY
        
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/sink.cpp" 

# src/sink/mqtt_sink HEADER_FILE_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/mqtt_sink/mqtt_service.hpp"

#src/sink/mqtt_sink SOURCE_FILES_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/mqtt_sink/mqtt_service.cpp"
        
# src/sink/rest_sink HEADER_FILE_ONLY
        
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/cached_file.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/file_cache.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/parameter.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/request.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/response.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/rest_service.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/routing.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/server.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/session.hpp"  
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/session_impl.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/tls_dector.hpp"
  
# src/sink/rest_sink SOURCE_FILES_ONLY

        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/file_cache.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/rest_service.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/server.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/session_impl.cpp"
  )

if(WITH_PYTHON)
  set(AGENT_SOURCES ${AGENT_SOURCES}
    
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/python/embedded.hpp"  
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/python/embedded.cpp"  
    )
endif()

if(WITH_RUBY)
  set(AGENT_SOURCES ${AGENT_SOURCES}    
# HEADER_FILE_ONLY      
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/ruby/embedded.hpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/ruby/ruby_agent.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/ruby/ruby_entity.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/ruby/ruby_observation.hpp"  
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/ruby/ruby_pipeline.hpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/ruby/ruby_smart_ptr.hpp"  
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/ruby/ruby_transform.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/ruby/ruby_type.hpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/ruby/ruby_vm.hpp"
        
#SOURCE_FILES_ONLY
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/ruby/embedded.cpp"  
    )
endif()

## configure a header file to pass some of the CMake settings to the source code
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../src/version.h.in" "${PROJECT_BINARY_DIR}/agent_lib/version.h")

set(AGENT_LIB_HEADERS ${AGENT_SOURCES})
list(FILTER AGENT_LIB_HEADERS INCLUDE REGEX "\\.(h|hpp|ipp)$")
set(AGENT_LIB_SOURCES ${AGENT_SOURCES})
list(FILTER AGENT_LIB_SOURCES INCLUDE REGEX "\\.(c|cpp)$")

# For IDE Grouping
source_group("Header Files")
source_group("Source Files")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/../src" PREFIX "Header Files" FILES ${AGENT_LIB_HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/../src" PREFIX "Source Files" FILES ${AGENT_LIB_SOURCES})

if(MSVC)
  if(CMAKE_GENERATOR_TOOLSET)
    if(${CMAKE_GENERATOR_TOOLSET} MATCHES "v14[01]_xp")
      message(info ": Setting /Ob1 for rest_sink/session_impl.cpp")
      set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/session_impl.cpp"
        PROPERTY COMPILE_FLAGS "/Ob1")
    endif()
  endif()

  # The modules including Beast required the /bigobj option in Windows
  set_property(SOURCE    
	"${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/mqtt_sink/mqtt_service.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/sink/rest_sink/session_impl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/mqtt/mqtt_adapter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/source/adapter/agent_adapter/agent_adapter.cpp"
    PROPERTY COMPILE_FLAGS "/bigobj")
endif()


add_library(agent_lib STATIC ${AGENT_SOURCES})
target_include_directories(
  agent_lib
  PUBLIC
    "${CONAN_INCLUDE_DIRS}"
    "${PROJECT_BINARY_DIR}/agent_lib"
    "${CMAKE_CURRENT_LIST_DIR}/../src"
    )

set_property(TARGET agent_lib PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  
target_link_libraries(
  agent_lib
  PUBLIC
  CONAN_PKG::boost CONAN_PKG::libxml2 CONAN_PKG::date CONAN_PKG::openssl 
  
  $<$<PLATFORM_ID:Linux>:pthread>
  $<$<PLATFORM_ID:Windows>:bcrypt>
  )

if(WITH_PYTHON)
  target_link_libraries(
    agent_lib
    PUBLIC
    Python3::Python)
endif()

if(WITH_RUBY)
  target_link_libraries(
    agent_lib
    PUBLIC
    CONAN_PKG::mruby)
endif()

target_compile_definitions(
  agent_lib
  PUBLIC
    $<$<BOOL:$ENV{APPVEYOR}>:APPVEYOR>
    $<$<PLATFORM_ID:Windows>:NOMINMAX>
    $<$<PLATFORM_ID:Windows>:WINVER=${WINVER}>
    $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=${WINVER}>
    MQTT_USE_TLS=ON
    MQTT_USE_WS=ON
    MQTT_USE_STR_CHECK=ON
    MQTT_STD_VARIANT
    MQTT_STD_OPTIONAL
    MQTT_STD_STRING_VIEW
    MQTT_USE_LOG
    BOOST_FILESYSTEM_VERSION=3
    )

if(WITH_PYTHON)
  target_compile_definitions(
    agent_lib
    PUBLIC
    WITH_PYTHON )
endif()

if(WITH_RUBY)
  target_compile_definitions(
    agent_lib
    PUBLIC
    WITH_RUBY )
endif()

if(AGENT_WITHOUT_IPV6)
  target_compile_definitions(
    agent_lib
    PUBLIC
    AGENT_WITHOUT_IPV6 )
endif()
  
# set_property(SOURCE ${AGENT_SOURCES} PROPERTY COMPILE_FLAGS_DEBUG "${COVERAGE_FLAGS}")
target_compile_features(agent_lib PUBLIC ${CXX_COMPILE_FEATURES})
target_clangformat_setup(agent_lib)
target_clangtidy_setup(agent_lib)
