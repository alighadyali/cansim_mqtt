cmake_minimum_required(VERSION 3.25)

set(APP_NAME cansim_mqtt)

project(${APP_NAME} VERSION 0.0.1 LANGUAGES CXX)

find_package(spdlog CONFIG REQUIRED)
set(PAHO_MQTT_C_LIBRARIES /opt/libs/paho-c/lib/libpaho-mqtt3as.a) 
set(PAHO_MQTT_C_INCLUDE_DIRS /opt/libs/paho-c/include)
list(APPEND CMAKE_PREFIX_PATH /opt/libs/paho-cpp)
# find_package(eclipse-paho-mqtt-c CONFIG REQUIRED)
find_package(PahoMqttCpp CONFIG REQUIRED)

set(SRC_FILES
    src/main.cpp
)

add_executable(${APP_NAME} ${SRC_FILES})

target_compile_options(${APP_NAME} PRIVATE
    ${COMPILER_OPTS})

target_link_libraries(
    ${APP_NAME}
    spdlog::spdlog
    # eclipse-paho-mqtt-c::paho-mqtt3as-static
    uuid
    PahoMqttCpp::paho-mqttpp3-static
)

# install files in output folder
install(TARGETS ${APP_NAME}
    RUNTIME DESTINATION bin
    COMPONENT bin)
