#include "bitmap.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include "lodepng.h"

using namespace std;

Bitmap::Bitmap(const string& file, const string& name)
: name(name)
{
    int w, h, channels;
    uint32_t* pixels = reinterpret_cast<uint32_t*>(stbi_load(file.data(), &w, &h, &channels, STBI_rgb_alpha));
    
    //Premultiply all the pixels by their alpha
    int count = w * h;
    uint32_t c,a,r,g,b;
    float m;
    for (int i = 0; i < count; ++i)
    {
        //abgr
        c = pixels[i];
        /*a = c & 0xff;
        m = static_cast<float>(a) / 255.0f;
        r = static_cast<uint32_t>(((c >> 24) & 0xff) * m);
        g = static_cast<uint32_t>(((c >> 16) & 0xff) * m);
        b = static_cast<uint32_t>(((c >> 8) & 0xff) * m);*/
        a = c >> 24;
        m = static_cast<float>(a) / 255.0f;
        r = static_cast<uint32_t>((c & 0xff) * m);
        g = static_cast<uint32_t>(((c >> 8) & 0xff) * m);
        b = static_cast<uint32_t>(((c >> 16) & 0xff) * m);
        pixels[i] = (r << 24) | (g << 16) | (b << 8) | a;
    }
    
    //TODO: skip if all corners contain opaque pixels
    
    //Get pixel bounds
    int minX = w - 1;
    int minY = h - 1;
    int maxX = 0;
    int maxY = 0;
    uint32_t p;
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            p = pixels[y * w + x];
            if ((p >> 24) > 0)
            {
                minX = min(x, minX);
                minY = min(y, minY);
                maxX = max(x, maxX);
                maxY = max(y, maxY);
            }
        }
    }
    if (maxX < minX || maxY < minY)
    {
        minX = 0;
        minY = 0;
        maxX = w - 1;
        maxY = h - 1;
        cout << "image is completely transparent: " << file << endl;
    }
    
    width = (maxX - minX) + 1;
    height = (maxY - minY) + 1;
    frameW = w;
    frameH = h;
    
    if (width == w && height == h)
    {
        frameX = 0;
        frameY = 0;
        data = pixels;
    }
    else
    {
        //Create the trimmed pixel array
        data = reinterpret_cast<uint32_t*>(malloc(width * height * sizeof(uint32_t)));
        frameX = -minX;
        frameY = -minY;
        
        //Copy trimmed pixels over to the trimmed pixel array
        for (int y = minY; y <= maxY; ++y)
            for (int x = minX; x <= maxX; ++x)
                data[(y - minY) * width + (x - minX)] = pixels[y * w + x];
        
        //Free the untrimmed pixels
        free(pixels);
    }
    
    /*string out = "bin/";
    out += name;
    stbi_write_png(out.data(), width, height, STBI_rgb_alpha, data, width * sizeof(uint32_t));*/
}

Bitmap::Bitmap(int width, int height)
: width(width), height(height)
{
    data = reinterpret_cast<uint32_t*>(calloc(width * height, sizeof(uint32_t)));
}

Bitmap::~Bitmap()
{
    free(data);
}

void Bitmap::SaveAs(const string& file)
{
    unsigned char* in = reinterpret_cast<unsigned char*>(data);
    lodepng::encode(file, in, width, height);
    //stbi_write_png(file.data(), width, height, STBI_rgb_alpha, data, width * sizeof(uint32_t));
}

void Bitmap::CopyPixels(const Bitmap* src, int tx, int ty)
{
    for (int y = 0; y < src->height; ++y)
        for (int x = 0; x < src->width; ++x)
            data[(ty + y) * width + (tx + x)] = src->data[y * src->width + x];
}
