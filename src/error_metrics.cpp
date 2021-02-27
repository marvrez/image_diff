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
