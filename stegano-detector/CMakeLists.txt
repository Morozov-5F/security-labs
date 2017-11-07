cmake_minimum_required(VERSION 3.8)
project(stegano_detector)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp)
set(EXECUTABLE_NAME stegano_detector)
add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})

find_package(JPEG REQUIRED)
if (JPEG_FOUND)
    include_directories(${JPEG_INCLUDE_DIR})
    target_link_libraries(${EXECUTABLE_NAME} ${JPEG_LIBRARIES})
endif()

# Input file
configure_file("source_image.jpg" "source_image.jpg" COPYONLY)