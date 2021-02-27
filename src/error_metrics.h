#ifndef ERROR_METRICS_H
#define ERROR_METRICS_H

#include <string>
#include <vector>

struct ErrorMetric {
    std::string short_name;
    std::string long_name;
};

static std::vector<ErrorMetric> g_error_metrics = {
    { "mse",  "Mean Squared Error" },
    { "rmse", "Relative Mean Squared Error" },
    { "mae",  "Mean Absolute Error" },
    { "mape", "Mean Absolute Percentage Error" },
};

#endif
