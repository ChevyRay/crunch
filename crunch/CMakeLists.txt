cmake_minimum_required(VERSION 3.8)

if(!MSVC)
set_compile_options(
    -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual
    -pedantic -fno-rtti -fPIE -fstack-protector-all
)
endif()

add_library(
    libcrunch
STATIC
    binary.cpp
    bitmap.cpp
    GuillotineBinPack.cpp
    hash.cpp
    lodepng.cpp
    MaxRectsBinPack.cpp
    packer.cpp
    Rect.cpp
    str.cpp
)
target_compile_features(libcrunch PUBLIC cxx_std_14)

add_executable(crunch main.cpp)
target_link_libraries(crunch libcrunch)
target_compile_features(crunch PUBLIC cxx_std_14)

install(TARGETS crunch DESTINATION bin)