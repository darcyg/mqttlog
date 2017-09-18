# Der Projektname
project(test_lib_mqtt_adapter)

# Minimal benötigte CMake-Version wenn z.B. bestimmte CMake-Kommandos benutzt werden
cmake_minimum_required(VERSION 2.6)

#external targets
add_custom_target(
   extern_lib_thread
   COMMAND make all
   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/../../../lib_thread/__gnumake
)

include_directories(${lib_mqtt_adapter_SOURCE_DIR} ${lib_os_system_SOURCE_DIR})
include_directories(${mosquitto_SOURCE_DIR}/lib ${mosquitto_SOURCE_DIR}/lib/cpp)
include_directories(${STDINT_H_PATH} ${PROJECT_SOURCE_DIR}/../../../lib_thread/sh_lib_thread)

#However, the file(GLOB...) allows for wildcard additions:
file(GLOB SOURCES "*.cpp")

#For the static library:
set(PROJECT_LINK_LIBS lib_mqtt_adapter$<$<CONFIG:Debug>:${CMAKE_DEBUG_POSTFIX}>)
set(PROJECT_LINK_LIB_THREAD lib_thread)

link_directories(${lib_mqtt_adapter_BINARY_DIR} ${mosquitto_BINARY_DIR}/lib ${mosquitto_BINARY_DIR}/lib/cpp )
link_directories(${PROJECT_SOURCE_DIR}/../../../lib_thread/__gnumake/x86)

#Generate the static library from the sources
add_executable(${PROJECT_NAME} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
add_dependencies(${PROJECT_NAME} lib_mqtt_adapter extern_lib_thread)
target_link_libraries(${PROJECT_NAME} ${PROJECT_LINK_LIBS} ${PROJECT_LINK_LIB_THREAD} pthread)

