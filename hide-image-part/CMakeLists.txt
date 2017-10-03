cmake_minimum_required(VERSION 3.8)

project(hide_image_part)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})
set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp)

# Prepare executable
set(EXECUTABLE_NAME "hide-image")
add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})

# Link against zlib
find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    target_link_libraries(${EXECUTABLE_NAME} ${ZLIB_LIBRARIES})
else()
    message(FATAL_ERROR "Unable to find zlib, aborting")
endif()

# Find all SFML stuff
find_package(SFML 2.4 REQUIRED system window graphics)
if (SFML_FOUND)
    include_directories(${SFML_INCLUDE_DIR})
    target_link_libraries(${EXECUTABLE_NAME} ${SFML_LIBRARIES} ${SFML_DEPENDENCIES})
else()
    message(FATAL_ERROR "Unable to find SFML packages, aborting")
endif()

# Prepare Feistel block cypher encryption library
set(FEISTEL_SOURCES feistel/FeistelNet.c)
set(FEISTEL_LIBRARY_NAME "feistel")
add_library(${FEISTEL_LIBRARY_NAME} ${FEISTEL_SOURCES})
target_link_libraries(${EXECUTABLE_NAME} ${FEISTEL_LIBRARY_NAME})

# Input file
configure_file("source_image.png" "source_image.png" COPYONLY)