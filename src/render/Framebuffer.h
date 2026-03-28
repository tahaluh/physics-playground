#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>

class Framebuffer
{
public:
    Framebuffer(int width, int height)
        : width(width), height(height), pixels(width * height, 0)
    {
    }

    void clear(uint32_t color)
    {
        std::fill(pixels.begin(), pixels.end(), color);
    }

    void setPixel(int x, int y, uint32_t color)
    {
        if (x < 0 || x >= width || y < 0 || y >= height)
            return;

        pixels[y * width + x] = color;
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
};