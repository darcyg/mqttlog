
#Enables Eclipse for automatic discovery of include directories
#set(CMAKE_VERBOSE_MAKEFILE on )

################################################################
#Target configuration
set(BLD_ARCH "bld_linux" CACHE STRING "Architecture descripion")

#Compiler flags

#The CDT Error Parser cannot handle error messages that span more than one line, 
#which is the default gcc behavior. In order to force gcc to generate single line 
#error messages with no line wrapping
IF(CMAKE_COMPILER_IS_GNUCC)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
ENDIF(CMAKE_COMPILER_IS_GNUCC)
IF(CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

################################################################
#Project
project(bld_device_cmake_LINUX)
# Minimal benötigte CMake-Version wenn z.B. bestimmte CMake-Kommandos benutzt werden
cmake_minimum_required(VERSION 3.5)
#Global enable of the -fPIC flag
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
#Set global attach of a "_g" postfix at debug build libraries 
set(CMAKE_DEBUG_POSTFIX _g)

include_directories(SYSTEM /usr/include)

################################################################
#List of Subdirectories

#find_package(uuid-dev REQUIRED)

add_subdirectory(int_os_basic)
add_subdirectory(int_comm)
add_subdirectory(sup_tools)
add_subdirectory(test_modules)
