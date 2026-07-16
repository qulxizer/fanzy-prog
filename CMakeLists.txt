cmake_minimum_required(VERSION 3.22)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
list(APPEND EXTRA_COMPONENT_DIRS components)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

idf_build_set_property(MINIMAL_BUILD ON)
project(fanzy-prog)

