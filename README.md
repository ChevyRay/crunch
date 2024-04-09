# crunch v0.13

This is a command line tool that will pack a bunch of images into a single, larger image. It was designed for [Celeste](http://www.celestegame.com/), but could be very helpful for other games.

It is designed using libraries with permissible licenses, so you are able to use it freely in your commercial and non-commercial projects. Please see each source file for its respective copyright and license.

## Features

- Export XML, JSON, or binary data
- Trim excess transparency
- Rotate images to fit
- Control atlas size and padding
- Premultiply pixel values
- Recursively scans folders
- Remove duplicate images
- Caching to prevent redundant builds
- Multi-image atlas when the sprites don't fit

## What does it do?

Given a folder with several images, like so:

```text
images/
    player.png
    tree.png
    enemy.png
```

It will output something like this:

```text
bin/
    images.png
    images.xml
    images.hash
```

Where `images.png` is the packed image, `images.xml` is an xml file describing where each sub-image is located, and `images.hash` is used for file caching (if none of the input files have changed since the last pack, the program will terminate).

There is also an option to use a binary format instead of xml.

## Usage

`crunch [OUTPUT] [INPUT1,INPUT2,INPUT3...] [OPTIONS...]`

For example...

`crunch bin/atlases/atlas assets/characters,assets/tiles -p -t -v -u -r -j`

This will output the following files:

```text
bin/atlases/atlas.png
bin/atlases/atlas.json
bin/atlases/atlas.hash
```

## Options

| option          | alias           | description     |
| --------------- | --------------- | --------------- |
| `--default`     | `-d`            | use default settings (`-x -p -t -u`) |
| `--xml`         | `-x`            | saves the atlas data as a `.xml` file |
| `--json`        | `-j`            | saves the atlas data as a `.json` file |
| `--binary`      | `-b`            | saves the atlas data as a `.bin` file |
| `--size N`      | `-s N`          | max atlas size (`N` can be `4096`, `2048`, `1024`, `512`, `256`, `128`, or `64`) |
| `--width N`     | `-w N`          | max atlas width (overrides `--size`) (`N` can be `4096`, `2048`, `1024`, `512`, `256`, `128`, or `64`) |
| `--height N`    | `-h N`          | max atlas height (overrides `--size`) (`N` can be `4096`, `2048`, `1024`, `512`, `256`, `128`, or `64`) |
| `--pad N`       | `-pd N`         | padding between images (`N` can be from `0` to `16`) |
| `--stretch N`   | `-st N`         | makes images' edges stretched by N pixels (`N` can be from `0` to `16`) |
| `--premultiply` | `-p`            | premultiplies the pixels of the bitmaps by their alpha channel |
| `--unique`      | `-u`            | remove duplicate bitmaps from the atlas |
| `--trim`        | `-t`            | trims excess transparency off the bitmaps |
| `--rotate`      | `-r`            | enabled rotating bitmaps 90 degrees clockwise when packing |
| `--heuristic`   | `-hr`           | use specific heuristic rule for packing images (`H` can be `bssf` (BestShortSideFit), `blsf` (BestLongSideFit), `baf` (BestAreaFit), `blr` (BottomLeftRule), `cpr` (ContactPointRule)) |
| `--binstr T`    | `-bs T`         | string type in binary format (`T` can be: `0` - null-termainated, `16` - prefixed (int16), `7` - 7-bit prefixed) |
| `--force`       | `-f`            | ignore caching, forcing the packer to repack |
| `--verbose`     | `-v`            | print to the debug console as the packer works |
| `--time`        | `-tm`           | use file's last write time instead of its content for hashing |
| `--split`       | `-sp`           | split output textures by subdirectories |
| `--nozero`      | `-nz`           | if there's only one packed texture, then zero at the end of its name will be omitted (ex. `images0.png` -> `images.png`) |

## Binary Format

```text
crch (0x68637263 in hex or 1751347811 in decimal (little endian))
[int16] version (current version is 0)
[byte] --trim enabled
[byte] --rotate enabled
[byte] string type (0 - null-termainated, 1 - prefixed (int16), 2 - 7-bit prefixed)
[int16] num_textures (below block is repeated this many times)
    [string] name
    [int16] num_images (below block is repeated this many times)
        [string] img_name
        [int16] img_x
        [int16] img_y
        [int16] img_width
        [int16] img_height
        [int16] img_frame_x         (if --trim enabled)
        [int16] img_frame_y         (if --trim enabled)
        [int16] img_frame_width     (if --trim enabled)
        [int16] img_frame_height    (if --trim enabled)
        [byte] img_rotated          (if --rotate enabled)
```

## Splitting

If `--split` (or `-sp`) is enabled output textures will be split by subdirectories.

For example:

`crunch bin/images images -b -sp` with input images

```text
images/
    chars/
        player.png
        enemy.png
    other/
        tree.png
        box.png
```

will output

```text
bin/
    images_chars.png (with player.png and enemy.png)
    images_chars.hash
    images_chars.bin
    images_other.png (with tree.png and box.png)
    images_other.hash
    images_other.bin
    images.bin
```

If `player.png` is the only changed image then only `images_chars.bin` will be packed
and `images_other.bin`  will be reused in `images.bin`.

This can be used for faster packing: unchanged subdirectories will be skipped
instead of packing unchanged images.

But there're some limitations:

- multiple inputs and images as inputs are not supported
- images in input directory itself will be ignored and not packed

## Building

### Windows

Create `build` directory:

```text
mkdir build
cd build
```

- Visual Studio:

```text
cmake -DCMAKE_BUILD_TYPE=Release ..
```

- MinGW:

```text
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=C:/your/mingw/path/gcc.exe -DCMAKE_CXX_COMPILER=C:/your/mingw/path/g++.exe -G "MinGW Makefiles" ..
```

Build command is same for both:

```text
cmake --build . --config Release
```

### Linux

```text
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

## License

Unless otherwise specified in a source file, everything in this project falls under the following license:

```text
MIT License

Copyright (c) 2017 Chevy Ray Johnston

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
