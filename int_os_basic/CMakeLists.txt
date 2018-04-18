project(os_basic)

add_subdirectory(lib_convention/sh_lib_convention)
add_subdirectory(lib_clock/sh_lib_clock)
add_subdirectory(lib_clock/sh_lib_clock_POSIX)
add_subdirectory(lib_log/sh_lib_log)
add_subdirectory(lib_thread/sh_lib_thread)
add_subdirectory(lib_thread/sh_lib_thread_POSIX)

set(TREADING_LIBS pthread rt)  
add_library(${PROJECT_NAME} SHARED lib_os_basic ) # $<TARGET_OBJECTS:lib_thread> 
  

target_link_libraries(${PROJECT_NAME} PUBLIC ${TREADING_LIBS} itf_lib_thread itf_lib_clock itf_lib_convention )
target_link_libraries(${PROJECT_NAME} PRIVATE "-Wl,--whole-archive" lib_clock lib_thread lib_log "-Wl,--no-whole-archive")
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_INCLUDE_HEADERS_1} ${lib_log_SOURCE_DIR} ${itf_lib_convention_SOURCE_DIR})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_COMPILE_DEF_1})

foreach(dir ${PROJECT_COMPILE_DEF_1})
  message(STATUS "dir='${dir}'")
endforeach()

#link_directories(${lib_log_BINARY_DIR})
#if (WIN32)
#    set_target_properties(${PROJECT_NAME} PROPERTIES
#        LINK_FLAGS "/WHOLEARCHIVE"
#    )
#elseif (APPLE)
#    set_target_properties(${PROJECT_NAME} PROPERTIES
#        LINK_FLAGS "-Wl,-all_load"
#    )
#else ()
#    set_target_properties(${PROJECT_NAME} PROPERTIES
#        LINK_FLAGS "-Wl,--whole-archive"
#    )
#endif ()
