cmake_minimum_required(VERSION 2.8)
project( camera_perspective )
find_package( OpenCV REQUIRED )
add_executable( camera_perspective camera_perspective.cpp )
target_link_libraries( camera_perspective ${OpenCV_LIBS} )
set(CMAKE_CXX_FLAGS "-std=c++0x")
