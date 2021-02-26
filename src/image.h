#ifndef IMAGE_H
#define IMAGE_H

#include <vector>
#include <cstdint>

class Image {
public:
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    const float* GetData() const { return m_data.data(); }
    float* GetData() { return m_data.data(); }

    static Image Create(uint32_t width, uint32_t height) { return Image(width, height); }
    static Image LoadFromFile(const char* filename);
    void SaveToFile(const char* filename);

private:
    static constexpr int NUM_CHANNELS = 4;
    uint32_t m_width, m_height;
    std::vector<float> m_data;

    Image(uint32_t width, uint32_t height) : m_width(width), m_height(height), m_data(width * height * NUM_CHANNELS, 0.0f) {}
};

#endif
