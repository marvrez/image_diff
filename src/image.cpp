#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <cstdio>
#include <cassert>

Image Image::LoadFromFile(const char* filename)
{
    int w, h, c;
    unsigned char* data = stbi_load(filename, &w, &h, &c, NUM_CHANNELS);
    if (!data) {
        fprintf(stderr, "Cannot load image \"%s\"\nSTB Reason: %s\n", filename, stbi_failure_reason());
        exit(0);
    }

    auto image = Image::Create(w, h);
    for (int i = 0; i < w * h * NUM_CHANNELS; ++i) {
        image.m_data[i] = (float)data[i] / 255.0f;
    }
    stbi_image_free(data);

    return image;
}

void Image::SaveToFile(const char* filename)
{
    std::vector<unsigned char> bytes(NUM_CHANNELS * m_width * m_height);
    for (uint32_t i = 0; i < m_width * m_height * NUM_CHANNELS; ++i) {
        bytes[i] = (unsigned char) (255 * m_data[i]);
    }
    int success = stbi_write_png(filename, m_width, m_height, NUM_CHANNELS, bytes.data(), NUM_CHANNELS * m_width);
    if(!success) fprintf(stderr, "Failed to write image %s\n", filename);
}
