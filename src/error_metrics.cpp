#include "error_metrics.h"

#include "image.h"

#include <cmath>

template<typename T> T Sqr(T x) { return x * x; }

namespace Metric {

double MSE::operator()(const float* a, const float* b, size_t count) const
{
    double error = 0.0;
    for (size_t i = 0; i < count; ++i) { error += Sqr(a[i] - b[i]); }
    return error / count;
}

double RMSE::operator()(const float* a, const float* b, size_t count) const
{
    double error = 0.0;
    for (size_t i = 0; i < count; ++i) { error += Sqr(a[i] - b[i]) / (Sqr(a[i]) + 1e-3); }
    return error / count;
}

double MAE::operator()(const float* a, const float* b, size_t count) const
{
    double error = 0.0;
    for (size_t i = 0; i < count; ++i) { error += std::fabs(Sqr(a[i] - b[i])); }
    return error / count;
}

double MAPE::operator()(const float* a, const float* b, size_t count) const
{
    double error = 0.0;
    for (size_t i = 0; i < count; ++i) { error += std::fabs((a[i] - b[i]) / a[i]); }
    return 0.0;
}

} // namespace Metric

template<typename ErrorMetric>
double ComputeError(const Image& img1, const Image& img2, float* error_map)
{
    ErrorMetric error_metric;
    constexpr size_t NUM_CHANNELS = 4;
    double sum = 0.0;
    const float* a = img1.GetData(), *b = img2.GetData();
    const size_t count = img1.GetWidth() * img1.GetHeight();
    for (size_t i = 0; i < count; ++i) {
        const auto error = error_metric(a, b, NUM_CHANNELS);
        if (error_map) *error_map++ = float(error);
        sum += error;
        a += NUM_CHANNELS, b += NUM_CHANNELS;
    }
    return sum / count;
}
template double ComputeError<Metric::MSE>(const Image& img1, const Image& img2, float* error_map);
template double ComputeError<Metric::RMSE>(const Image& img1, const Image& img2, float* error_map);
template double ComputeError<Metric::MAE>(const Image& img1, const Image& img2, float* error_map);
template double ComputeError<Metric::MAPE>(const Image& img1, const Image& img2, float* error_map);
