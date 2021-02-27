#ifndef ERROR_METRICS_H
#define ERROR_METRICS_H

#include <string>
#include <vector>

struct ErrorMetric {
    std::string short_name;
    std::string long_name;
};

namespace Metric {
struct MetricFunction {
    virtual double operator()(const float* a, const float* b, size_t count) const = 0;
};
struct MSE : public MetricFunction {
    virtual double operator()(const float* a, const float* b, size_t count) const;
};
struct RMSE : public MetricFunction {
    virtual double operator()(const float* a, const float* b, size_t count) const;
};
struct MAE : public MetricFunction {
    virtual double operator()(const float* a, const float* b, size_t count) const;
};
struct MAPE : public MetricFunction {
    virtual double operator()(const float* a, const float* b, size_t count) const;
};

} // namespace Metric

static std::vector<ErrorMetric> g_error_metrics = {
    { "mse",  "Mean Squared Error" },
    { "rmse", "Relative Mean Squared Error" },
    { "mae",  "Mean Absolute Error" },
    { "mape", "Mean Absolute Percentage Error" },
};

#endif
