project(lib_os_system)

#However, the file(GLOB...) allows for wildcard additions:
file(GLOB SOURCES "*.cpp")

#Generate the static library from the sources
add_library(${PROJECT_NAME} STATIC ${SOURCES})