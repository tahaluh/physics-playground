#pragma once

#include <cstdint>
#include <limits>
#include <vector>
#include <algorithm>

class Framebuffer
{
public:
    Framebuffer(int width, int height)
        : width(width), height(height), pixels(width * height, 0), depth(width * height, std::numeric_limits<float>::infinity())
    {
    }

    void clear(uint32_t color)
    {
        std::fill(pixels.begin(), pixels.end(), color);
    }

    void clearDepth(float value = std::numeric_limits<float>::infinity())
    {
        std::fill(depth.begin(), depth.end(), value);
    }

    void setPixel(int x, int y, uint32_t color)
    {
        if (x < 0 || x >= width || y < 0 || y >= height)
            return;

        pixels[y * width + x] = color;
    }

    bool setPixelDepth(int x, int y, float z, uint32_t color)
    {
        if (x < 0 || x >= width || y < 0 || y >= height)
            return false;

        const int index = y * width + x;
        if (z >= depth[index])
            return false;

        depth[index] = z;
        pixels[index] = color;
        return true;
    }

    const uint32_t *data() const
    {
        return pixels.data();
    }

    uint32_t *data()
    {
        return pixels.data();
    }

    int getWidth() const
    {
        return width;
    }

    int getHeight() const
    {
        return height;
    }

private:
    int width;
    int height;
    std::vector<uint32_t> pixels;
    std::vector<float> depth;
};
