cmake_minimum_required (VERSION 2.8.11)
project (crunch)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include_directories(../Unpacker ${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCES
crunch/MaxRectsBinPack.cpp
crunch/Rect.cpp
crunch/lodepng.cpp
crunch/hash.cpp
crunch/bitmap.cpp
crunch/str.cpp
crunch/binary.cpp
crunch/packer.cpp
crunch/main.cpp
crunch/GuillotineBinPack.cpp
)

add_library(crunch ${SOURCES})

add_executable(crunch_gen main_real.cpp)
target_link_libraries(crunch_gen crunch)
