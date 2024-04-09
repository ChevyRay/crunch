cmake_minimum_required(VERSION 3.1.2)

project(crunch VERSION 0.12 LANGUAGES C CXX)

add_executable(crunch 
    crunch/main.cpp
    
    crunch/third_party/lodepng.cpp
    crunch/third_party/MaxRectsBinPack.cpp
    crunch/third_party/Rect.cpp
    
    crunch/binary.cpp
    crunch/bitmap.cpp
    crunch/cli.cpp
    crunch/hash.cpp
    crunch/options.cpp
    crunch/packer.cpp
    )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
target_compile_features(crunch PUBLIC cxx_std_20)
