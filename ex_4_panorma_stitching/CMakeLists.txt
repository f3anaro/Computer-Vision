cmake_minimum_required(VERSION 2.8)
project( panorama )
find_package( OpenCV REQUIRED )
add_executable( panorama
    src/panorama.cpp
    src/homographies.cpp
    src/local_maxima.cpp
    src/matching.cpp
    src/render.cpp
    src/saves.cpp
)
target_link_libraries( panorama ${OpenCV_LIBS} )
set(CMAKE_CXX_FLAGS "-std=c++0x")
