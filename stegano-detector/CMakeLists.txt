cmake_minimum_required(VERSION 3.8)
project(stegano_detector)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp)
set(EXECUTABLE_NAME stegano_detector)
add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})

find_package(JPEG REQUIRED)
if (JPEG_FOUND)
    set(JPEG_INCLUDE_DIR_INT, "/usr/local/include/")
    message("JPEG include directory: ${JPEG_INCLUDE_DIR_INT}")
    message("JPEG libraries        : ${JPEG_LIBRARIES}")
    include_directories(${JPEG_INCLUDE_DIR_INT})
    target_link_libraries(${EXECUTABLE_NAME} ${JPEG_LIBRARIES})
endif()

find_package(FANN REQUIRED)
if (FANN_FOUND)
    include_directories(${FANN_INCLUDE_DIR})
    target_link_libraries(${EXECUTABLE_NAME} ${FANN_LIBRARIES})
endif()

# Input file
configure_file("source_image.jpg" "source_image.jpg" COPYONLY)
configure_file("result_image.jpg" "result_image.jpg" COPYONLY)