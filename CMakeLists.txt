project(vlc_rtsp_test)
cmake_minimum_required(VERSION 2.8)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++11")

find_package(OpenCV REQUIRED)

aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} vlc)
target_link_libraries(${PROJECT_NAME} opencv_core opencv_highgui)
