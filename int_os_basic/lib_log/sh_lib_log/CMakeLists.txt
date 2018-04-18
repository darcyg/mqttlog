project(lib_log)

#However, the file(GLOB...) allows for wildcard additions:
file(GLOB SOURCES "*.c")


add_definitions(-DTRACE)

#Generate the static library from the sources
add_library(${PROJECT_NAME} STATIC ${SOURCES})
 
if(${BLD_ARCH} STREQUAL "bld_stm32")
    target_link_libraries(${PROJECT_NAME} dev_trace_CORTEXM) 
ELSE()
    target_link_libraries(${PROJECT_NAME} itf_lib_clock)
ENDIF()

target_include_directories(${PROJECT_NAME} PUBLIC ./)