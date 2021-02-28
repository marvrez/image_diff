#ifndef ERROR_METRICS_H
#define ERROR_METRICS_H

#include <string>
#include <vector>
#include <functional>

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
using ErrorFunction = std::function<double(const Image& img1, const Image& img2, float* error)>;
} // namespace Metric

template<typename ErrorMetric>
double ComputeError(const Image& img1, const Image& img2, float* error_map);

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
};

#endif
