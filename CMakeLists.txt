cmake_minimum_required(VERSION 3.10)

project(RmlOgre VERSION 0.1 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(OGRE REQUIRED)
find_package(RmlUi REQUIRED)


add_subdirectory("example")
