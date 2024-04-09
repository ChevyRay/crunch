#ifndef cli_hpp
#define cli_hpp

#include <iostream>
#include <string>

using namespace std;

const string version = "v0.13";

static const string helpMessage = R"(
usage:
  crunch [OUTPUT] [INPUT1,INPUT2,INPUT3...] [OPTIONS...]
    
example:
  crunch bin/atlases/atlas assets/characters,assets/tiles -p -t -v -u -r
    
options:
  name           | alias |
  -----------------------------------------------------------------------------------------------------------------------------------------------
  --default      |  -d   |  use default settings (-x -p -t -u)
  -----------------------------------------------------------------------------------------------------------------------------------------------
  --xml          |  -x   |  saves the atlas data as a .xml file
  --json         |  -j   |  saves the atlas data as a .json file
  --binary       |  -b   |  saves the atlas data as a .bin file
  -----------------------------------------------------------------------------------------------------------------------------------------------
  --size N       |  -s   |  max atlas size (N can be 4096, 2048, 1024, 512, 256, 128, or 64)
  --width N      |  -w   |  max atlas width (overrides --size) (N can be 4096, 2048, 1024, 512, 256, 128, or 64)
  --height N     |  -h   |  max atlas height (overrides --size) (N can be 4096, 2048, 1024, 512, 256, 128, or 64)
  --padding N    |  -pd  |  padding between images (N can be from 0 to 16)
  --stretch N    |  -st  |  makes images' edges stretched by N pixels (N can be from 0 to 16)
  -----------------------------------------------------------------------------------------------------------------------------------------------
  --premultiply  |  -p   |  premultiplies the pixels of the bitmaps by their alpha channel
  --unique       |  -u   |  remove duplicate bitmaps from the atlas
  --trim         |  -t   |  trims excess transparency off the bitmaps
  --rotate       |  -r   |  enabled rotating bitmaps 90 degrees clockwise when packing
  --heuristic H  |  -hr  |  use specific heuristic rule for packing images (H can be bssf (BestShortSideFit), blsf (BestLongSideFit), baf (BestAreaFit), blr (BottomLeftRule), cpr (ContactPointRule))
  -----------------------------------------------------------------------------------------------------------------------------------------------
  --binstr T     |  -bs  |  string type in binary format (T can be: 0 - null-termainated, 16 - prefixed (int16), 7 - 7-bit prefixed)
  --force        |  -f   |  ignore the hash, forcing the packer to repack
  --verbose      |  -v   |  print to the debug console as the packer works
  --time         |  -tm  |  use file's last write time instead of its content for hashing
  --split        |  -sp  |  split output textures by subdirectories
  --nozero       |  -nz  |  if there's ony one packed texture, then zero at the end of its name will be omitted (ex. images0.png -> images.png)
    
binary format:
  crch (0x68637263 in hex or 1751347811 in decimal)
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
    )";

void PrintHelp(int argc, const char *argv[]);
void ParseArguments(int argc, const char *argv[], int offset);

#endif
