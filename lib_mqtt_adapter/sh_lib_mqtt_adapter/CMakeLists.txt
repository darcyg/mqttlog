# Der Projektname
project(lib_mqtt_adapter)
# Minimal benötigte CMake-Version wenn z.B. bestimmte CMake-Kommandos benutzt werden
cmake_minimum_required(VERSION 2.6)

macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "--std=gnu++11 ${CMAKE_CXX_FLAGS}")
    endif ()
  else ()
    set (CMAKE_CXX_STANDARD 11)
    set (CMAKE_CXX_STANDARD_REQUIRED ON)
  endif ()
endmacro(use_cxx11)


use_cxx11()

set (CMAKE_CXX_FLAGS "-fPIC ${CMAKE_CXX_FLAGS}")

SET(CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_DEBUG_POSTFIX _g)

add_subdirectory(sh_lib_mosquitto)
add_subdirectory(sh_lib_os_system)
add_subdirectory (va_test_mqtt_adapter) 

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/../../lib_convention/sh_lib_convention ${lib_os_system_SOURCE_DIR}) 
include_directories(${mosquitto_SOURCE_DIR}/lib ${mosquitto_SOURCE_DIR}/lib/cpp ${STDBOOL_H_PATH})

#However, the file(GLOB...) allows for wildcard additions:
file(GLOB SOURCES "*.cpp")

set(PROJECT_LINK_LIB_OS_SYSTEM lib_os_system$<$<CONFIG:Debug>:${CMAKE_DEBUG_POSTFIX}>)
set(PROJECT_LINK_LIB_MOSQUITTO mosquitto$<$<CONFIG:Debug>:${CMAKE_DEBUG_POSTFIX}>)
set(PROJECT_LINK_LIB_MOSQUITTOPP mosquittopp$<$<CONFIG:Debug>:${CMAKE_DEBUG_POSTFIX}>)

link_directories(${lib_os_system_BINARY_DIR} ${mosquitto_BINARY_DIR}/lib ${mosquitto_BINARY_DIR}/lib/cpp)

#Generate the static library from the sources
add_library(lib_mqtt_adapter SHARED ${SOURCES})
add_dependencies(lib_mqtt_adapter lib_os_system mosquitto libmosquitto mosquittopp)
target_link_libraries(lib_mqtt_adapter -Wl,--start-group ${PROJECT_LINK_LIB_OS_SYSTEM} ${PROJECT_LINK_LIB_MOSQUITTO} ${PROJECT_LINK_LIB_MOSQUITTOPP} pthread -Wl,--end-group)
