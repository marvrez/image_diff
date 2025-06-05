#ifndef ERROR_METRICS_H
#define ERROR_METRICS_H

#include <string>
#include <vector>
#include <functional>
#include <cmath> // For log10

class Image;

namespace Metric {
struct ErrorFunctor {
    virtual double operator()(const float* a, const float* b, size_t count) const = 0;
};
struct MSE : public ErrorFunctor {
    virtual double operator()(const float* a, const float* b, size_t count) const;
};
struct RMSE : public ErrorFunctor {
    virtual double operator()(const float* a, const float* b, size_t count) const;
};
struct MAE : public ErrorFunctor {
    virtual double operator()(const float* a, const float* b, size_t count) const;
};
struct MAPE : public ErrorFunctor {
    virtual double operator()(const float* a, const float* b, size_t count) const;
};
// Peak Signal-to-Noise Ratio
struct PSNR : public ErrorFunctor {
    virtual double operator()(const float* a, const float* b, size_t count) const;
};
struct SSIM : public ErrorFunctor {
    // This operator might not do much if we specialize ComputeError<SSIM>
    // or it could be used to signal that this is an SSIM metric.
    // For now, let's define it like others.
    virtual double operator()(const float* a, const float* b, size_t count) const;
};
struct ColorHistogramDistance : public ErrorFunctor {
    virtual double operator()(const float* a, const float* b, size_t count) const;
};
struct CIEDE2000 : public ErrorFunctor {
    virtual double operator()(const float* a, const float* b, size_t count) const;
};
struct NCC : public ErrorFunctor {
    virtual double operator()(const float* a, const float* b, size_t count) const;
};
using ErrorFunction = std::function<double(const Image& img1, const Image& img2, float* error)>;
} // namespace Metric

template<typename ErrorMetric>
double ComputeError(const Image& img1, const Image& img2, float* error_map);

// Forward declarations for ComputeError template specializations
// Definitions are in error_metrics.cpp
template<>
double ComputeError<Metric::SSIM>(const Image& img1, const Image& img2, float* error_map);
template<>
double ComputeError<Metric::ColorHistogramDistance>(const Image& img1, const Image& img2, float* error_map);
template<>
double ComputeError<Metric::NCC>(const Image& img1, const Image& img2, float* error_map);
// Note: CIEDE2000 uses the generic ComputeError with its specific operator(), so no specialization needed here.

struct ErrorMetric {
    std::string short_name;
    std::string long_name;
    Metric::ErrorFunction ComputeError;
};

static std::vector<ErrorMetric> g_error_metrics = {
    { "mse",  "Mean Squared Error",             ComputeError<Metric::MSE>  },
    { "rmse", "Relative Mean Squared Error",    ComputeError<Metric::RMSE> },
    { "mae",  "Mean Absolute Error",            ComputeError<Metric::MAE> },
    { "mape", "Mean Absolute Percentage Error", ComputeError<Metric::MAPE> },
    { "psnr", "Peak Signal-to-Noise Ratio",   ComputeError<Metric::PSNR>  },
    { "ssim", "Structural Similarity Index",  ComputeError<Metric::SSIM>  },
    { "histdist", "Color Histogram Distance", ComputeError<Metric::ColorHistogramDistance> },
    { "ciede2000", "CIEDE2000 Color Difference", ComputeError<Metric::CIEDE2000> },
    { "ncc", "Normalized Cross-Correlation", ComputeError<Metric::NCC> },
};

#endif
