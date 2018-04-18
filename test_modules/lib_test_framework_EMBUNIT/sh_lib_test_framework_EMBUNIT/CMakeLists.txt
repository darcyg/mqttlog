project(lib_test_framework_EMBUNIT)

#However, the file(GLOB...) allows for wildcard additions:
file(GLOB SOURCES "*.c")



#Generate the static library from the sources
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ./)

if(TARGET dev_trace_CORTEXM)
   message(STATUS "dev_trace_CORTEXM output")
   target_link_libraries(${PROJECT_NAME} dev_trace_CORTEXM)
   target_compile_definitions(${PROJECT_NAME} PRIVATE DEV_TRACE_VPRINTF DEV_TRACE_PRINTF)
endif()

#target_link_libraries(${PROJECT_NAME} lib_convention itf_lib_clock)