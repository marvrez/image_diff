#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>

#include "argparse.hpp"

#include "image.h"
#include "error_metrics.h"

template<typename T> T Lerp(T a, T b, T t) { return a + t * (b - a); }
template<typename T> T Clamp(T x, T lo, T hi) { return std::max(lo, std::min(hi, x)); }

void PrintMetrics()
{
    std::cout << "Available error metrics:" << std::endl;
    for (auto& metric : g_error_metrics) {
        std::cout << "  " << std::left << std::setw(6) << metric.short_name << "(" << metric.long_name << ")" << std::endl;
    }
}

Image GenerateHeatMap(uint32_t width, uint32_t height, const float* error)
{
    static constexpr float HEATMAP_COLORS[5][3] = {
        { 0.f, 0.f, 1.f }, // t = 0.00, blue
        { 0.f, 1.f, 1.f }, // t = 0.25, cyan
        { 0.f, 1.f, 0.f }, // t = 0.50, green
        { 1.f, 1.f, 0.f }, // t = 0.75, yellow
        { 1.f, 0.f, 0.f }, // t = 1.00, red
    };

    const auto [min_val, max_val] = std::minmax_element(error, error + width * height);
    const float range = std::max(1e-6f, *max_val - *min_val);

    auto heatmap = Image::Create(width, height);
    float* dst = heatmap.GetData();
    for (size_t i = 0; i < width * height; ++i) {
        const float t = Clamp((error[i] - *min_val) / range, 0.0f, 1.0f);
        const int c = Clamp(int(std::floor(t * 4.0f)), 0, 3);
        for (size_t i = 0; i < 3; ++i) *dst++ = Lerp(HEATMAP_COLORS[c][i], HEATMAP_COLORS[c + 1][i], t * 4.0f - c);
        *dst++ = 1.0f;
    }
    return heatmap;
}

int main(int argc, char** argv)
{
    argparse::ArgumentParser program("image_diff");
    program.add_argument("-m", "--metric").required().help("Error metric")
        .action([](const std::string& name) {
            auto it = std::find_if(g_error_metrics.begin(), g_error_metrics.end(), [&name] (const ErrorMetric& metric) { return metric.short_name == name; });
            if (it != g_error_metrics.end()) return *it;
            std::cerr << "Unknown error metric '" << name << "'." << std::endl;
            PrintMetrics();
            exit(0);
        });
    program.add_argument("-t", "--threshold").default_value(0.0f).help("Error threshold");
    program.add_argument("-e", "--heatmap").default_value(false).implicit_value(true).help("Error threshold");
    program.add_argument("image1").help("The first image").required();
    program.add_argument("image2").help("The second image").required();

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        std::cout << program;
        exit(0);
    }

    // Load images.
    const auto img1 = Image::LoadFromFile(program.get<std::string>("image1").c_str());
    const auto img2 = Image::LoadFromFile(program.get<std::string>("image2").c_str());

    // Check resolution.
    if (img1.GetWidth() != img2.GetWidth() || img1.GetHeight() != img2.GetHeight()) {
        std::cout << "Cannot compare images of different resolutions." << std::endl;
        return 1;
    }
    const uint32_t width = img1.GetWidth(), height = img2.GetHeight();

    // Compare images.
    const auto generate_heatmap = program.get<bool>("--heatmap");
    std::unique_ptr<float[]> error_map = generate_heatmap ? std::make_unique<float[]>(width * height) : nullptr;
    const auto metric = program.get<ErrorMetric>("--metric");
    const auto error = metric.ComputeError(img1, img2, error_map.get());

    // Generate heat map.
    if (error_map) {
        const auto heatmap = GenerateHeatMap(width, height, error_map.get());
        heatmap.SaveToFile("heatmap.png");
    }

    std::cout << "Computed Error: " << error << std::endl;
    const auto threshold = program.get<float>("--threshold");
    if (std::isnan(error) || std::isinf(error) || error > threshold) return 1;

    return 0;
}
