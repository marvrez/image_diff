#include "error_metrics.h"

#include "image.h"

#include <cmath>
#include <limits> // For std::numeric_limits
#include <vector> // For SSIM helper data structures
#include <numeric> // For std::accumulate in SSIM helpers

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

double Metric::PSNR::operator()(const float* a, const float* b, size_t count) const
{
    double mse = 0.0;
    for (size_t i = 0; i < count; ++i) {
        mse += Sqr(a[i] - b[i]);
    }
    mse /= count;

    if (mse == 0.0) {
        return std::numeric_limits<double>::infinity();
    }
    // MAX_I is 1.0f because image data is normalized to [0, 1]
    // PSNR = 20 * log10(MAX_I) - 10 * log10(MSE)
    // Since MAX_I = 1.0, log10(MAX_I) = 0
    // So, PSNR = -10 * log10(MSE)
    return -10.0 * std::log10(mse);
}

double Metric::SSIM::operator()(const float* a, const float* b, size_t count) const {
    // The actual SSIM calculation is too complex for a pixel-wise functor here
    // if we want to avoid redundant calculations across overlapping windows
    // when called by the generic ComputeError.
    // We will provide a full template specialization for ComputeError<Metric::SSIM>.
    // This function will likely not be directly called if the specialization is correct.
    return 0.0; // Placeholder
}

double Metric::ColorHistogramDistance::operator()(const float* a, const float* b, size_t count) const {
    // Actual calculation will be in the specialized ComputeError.
    return 0.0; // Placeholder
}

// double Metric::CIEDE2000::operator()(const float* a_rgb, const float* b_rgb, size_t count) const; // Redundant declaration removed

double Metric::NCC::operator()(const float* a, const float* b, size_t count) const {
    // Actual calculation will be in the specialized ComputeError for global NCC.
    return 0.0; // Placeholder
}

} // namespace Metric

namespace { // Anonymous namespace for helpers

// Helper to get pixel value, handling boundary conditions (e.g., replicate)
// Assumes 4 channels (RGBA), we only care about luminance for classic SSIM.
// For simplicity, this example will average R,G,B for luminance.
// A more correct approach would use Y channel from YUV or specific luminance weights.
float get_luminance(const float* pixel_ptr) {
    return (pixel_ptr[0] + pixel_ptr[1] + pixel_ptr[2]) / 3.0f;
}

// Calculates mean of a window (luminance values)
double calculate_mean(const float* img_data, int img_width, int img_height,
                       int win_x, int win_y, int win_size, int num_channels) {
    double sum = 0.0;
    int count = 0;
    for (int r = 0; r < win_size; ++r) {
        for (int c = 0; c < win_size; ++c) {
            int current_y = win_y + r;
            int current_x = win_x + c;
            // Basic boundary handling: skip pixels outside image
            if (current_x >= 0 && current_x < img_width && current_y >= 0 && current_y < img_height) {
                const float* pixel = img_data + (current_y * img_width + current_x) * num_channels;
                sum += get_luminance(pixel);
                count++;
            }
        }
    }
    return (count > 0) ? sum / count : 0.0;
}

// Calculates variance of a window
double calculate_variance(const float* img_data, int img_width, int img_height,
                           int win_x, int win_y, int win_size, int num_channels, double mean) {
    double sum_sq_diff = 0.0;
    int count = 0;
    for (int r = 0; r < win_size; ++r) {
        for (int c = 0; c < win_size; ++c) {
            int current_y = win_y + r;
            int current_x = win_x + c;
            if (current_x >= 0 && current_x < img_width && current_y >= 0 && current_y < img_height) {
                const float* pixel = img_data + (current_y * img_width + current_x) * num_channels;
                double lum = get_luminance(pixel);
                sum_sq_diff += (lum - mean) * (lum - mean);
                count++;
            }
        }
    }
    return (count > 0) ? sum_sq_diff / count : 0.0; // Use count, not count-1 for population variance estimate used in SSIM
}

// Calculates covariance of two windows
double calculate_covariance(const float* img1_data, const float* img2_data,
                             int img_width, int img_height,
                             int win_x, int win_y, int win_size, int num_channels,
                             double mean1, double mean2) {
    double sum_prod_diff = 0.0;
    int count = 0;
    for (int r = 0; r < win_size; ++r) {
        for (int c = 0; c < win_size; ++c) {
            int current_y = win_y + r;
            int current_x = win_x + c;
             if (current_x >= 0 && current_x < img_width && current_y >= 0 && current_y < img_height) {
                const float* pixel1 = img1_data + (current_y * img_width + current_x) * num_channels;
                const float* pixel2 = img2_data + (current_y * img_width + current_x) * num_channels;
                double lum1 = get_luminance(pixel1);
                double lum2 = get_luminance(pixel2);
                sum_prod_diff += (lum1 - mean1) * (lum2 - mean2);
                count++;
            }
        }
    }
    return (count > 0) ? sum_prod_diff / count : 0.0;
}

const int HISTOGRAM_BINS = 256;

// Calculates a 1D histogram for a single color channel
void calculate_histogram_channel(const float* img_data, int width, int height, int num_channels, int channel_idx, std::vector<double>& hist) {
    hist.assign(HISTOGRAM_BINS, 0.0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float* pixel = img_data + (y * width + x) * num_channels;
            // Ensure pixel value is in [0, 1] range, then scale to bin index
            float val = pixel[channel_idx];
            if (val < 0.0f) val = 0.0f;
            if (val > 1.0f) val = 1.0f;
            int bin = static_cast<int>(val * (HISTOGRAM_BINS - 1));
            hist[bin]++;
        }
    }
    // Normalize histogram (sum to 1)
    double sum = 0.0;
    for(double h_val : hist) sum += h_val;
    if (sum > 0) {
        for(double& h_val : hist) h_val /= sum;
    }
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// sRGB to XYZ conversion matrix (D65 illuminant)
// Assumes input RGB values are linear (not gamma corrected).
// Our image loader converts to float 0-1, let's assume they are linear here.
// If they were gamma-corrected, they'd need to be linearized first.
void rgb_to_xyz(float r, float g, float b, double& x, double& y, double& z) {
    // Inverse gamma correction (simplified - assuming linear input for now)
    // If proper sRGB, r = (r > 0.04045) ? pow((r + 0.055) / 1.055, 2.4) : r / 12.92; etc.
    // For now, assume r,g,b are already linear.

    x = 0.4124564 * r + 0.3575761 * g + 0.1804375 * b;
    y = 0.2126729 * r + 0.7151522 * g + 0.0721750 * b; // This is also Luminance
    z = 0.0193339 * r + 0.1191920 * g + 0.9503041 * b;
}

// XYZ to Lab conversion (D65 illuminant)
void xyz_to_lab(double x, double y, double z, double& l_star, double& a_star, double& b_star) {
    // Observer= 2°, Illuminant= D65
    double ref_x = 0.95047; // Xn
    double ref_y = 1.00000; // Yn
    double ref_z = 1.08883; // Zn

    x /= ref_x;
    y /= ref_y;
    z /= ref_z;

    auto f = [](double t) {
        return (t > 0.008856) ? std::cbrt(t) : (7.787 * t + 16.0 / 116.0);
    };

    double fx = f(x);
    double fy = f(y);
    double fz = f(z);

    l_star = (116.0 * fy) - 16.0;
    a_star = 500.0 * (fx - fy);
    b_star = 200.0 * (fy - fz);
}

// CIEDE2000 implementation
double ciede2000_delta_e(double l1, double a1, double b1, double l2, double a2, double b2) {
    double c1 = std::sqrt(a1 * a1 + b1 * b1);
    double c2 = std::sqrt(a2 * a2 + b2 * b2);
    double c_bar = (c1 + c2) / 2.0;

    double g = 0.5 * (1.0 - std::sqrt(std::pow(c_bar, 7.0) / (std::pow(c_bar, 7.0) + std::pow(25.0, 7.0))));
    double a1_prime = (1.0 + g) * a1;
    double a2_prime = (1.0 + g) * a2;

    double c1_prime = std::sqrt(a1_prime * a1_prime + b1 * b1);
    double c2_prime = std::sqrt(a2_prime * a2_prime + b2 * b2);
    double c_bar_prime = (c1_prime + c2_prime) / 2.0;

    double h1_prime_rad = std::atan2(b1, a1_prime);
    if (h1_prime_rad < 0) h1_prime_rad += 2.0 * M_PI;
    double h2_prime_rad = std::atan2(b2, a2_prime);
    if (h2_prime_rad < 0) h2_prime_rad += 2.0 * M_PI;

    double delta_l_prime = l2 - l1;
    double delta_c_prime = c2_prime - c1_prime;

    double delta_h_prime_val;
    if (c1_prime * c2_prime == 0) {
        delta_h_prime_val = 0;
    } else {
        double diff_h = h2_prime_rad - h1_prime_rad;
        if (std::abs(diff_h) <= M_PI) {
            delta_h_prime_val = diff_h;
        } else if (diff_h > M_PI) {
            delta_h_prime_val = diff_h - 2.0 * M_PI;
        } else {
            delta_h_prime_val = diff_h + 2.0 * M_PI;
        }
    }
    double delta_H_prime = 2.0 * std::sqrt(c1_prime * c2_prime) * std::sin(delta_h_prime_val / 2.0);

    double l_bar_prime = (l1 + l2) / 2.0;
    // c_bar_prime defined above
    double h_bar_prime_rad;
     if (c1_prime * c2_prime == 0) {
        h_bar_prime_rad = h1_prime_rad + h2_prime_rad;
    } else {
        double diff_h_abs = std::abs(h1_prime_rad - h2_prime_rad);
        if (diff_h_abs <= M_PI) {
            h_bar_prime_rad = (h1_prime_rad + h2_prime_rad) / 2.0;
        } else {
            if ((h1_prime_rad + h2_prime_rad) < 2.0 * M_PI) {
                h_bar_prime_rad = (h1_prime_rad + h2_prime_rad + 2.0 * M_PI) / 2.0;
            } else {
                h_bar_prime_rad = (h1_prime_rad + h2_prime_rad - 2.0 * M_PI) / 2.0;
            }
        }
    }

    double t_val = 1.0 - 0.17 * std::cos(h_bar_prime_rad - M_PI / 6.0) +
                   0.24 * std::cos(2.0 * h_bar_prime_rad) +
                   0.32 * std::cos(3.0 * h_bar_prime_rad + M_PI / 30.0) -
                   0.20 * std::cos(4.0 * h_bar_prime_rad - 63.0 * M_PI / 180.0); // 63 deg to rad

    double delta_theta_rad = (30.0 * M_PI / 180.0) * std::exp(-std::pow((h_bar_prime_rad * 180.0 / M_PI - 275.0) / 25.0, 2.0));

    double r_c = 2.0 * std::sqrt(std::pow(c_bar_prime, 7.0) / (std::pow(c_bar_prime, 7.0) + std::pow(25.0, 7.0)));
    double s_l = 1.0 + (0.015 * std::pow(l_bar_prime - 50.0, 2.0)) / std::sqrt(20.0 + std::pow(l_bar_prime - 50.0, 2.0));
    double s_c = 1.0 + 0.045 * c_bar_prime;
    double s_h = 1.0 + 0.015 * c_bar_prime * t_val;
    double r_t = -std::sin(2.0 * delta_theta_rad) * r_c;

    // Parametric weighting factors (typically 1)
    double k_l = 1.0, k_c = 1.0, k_h = 1.0;

    double delta_e = std::sqrt(
        std::pow(delta_l_prime / (k_l * s_l), 2.0) +
        std::pow(delta_c_prime / (k_c * s_c), 2.0) +
        std::pow(delta_H_prime / (k_h * s_h), 2.0) +
        r_t * (delta_c_prime / (k_c * s_c)) * (delta_H_prime / (k_h * s_h))
    );
    return delta_e;
}

// Using a simple average for grayscale. Could use luminance weights (0.299R + 0.587G + 0.114B)
float get_grayscale(const float* pixel_rgb) {
    return (pixel_rgb[0] + pixel_rgb[1] + pixel_rgb[2]) / 3.0f;
}

// Calculates Chi-Squared distance between two normalized histograms
double chi_squared_distance(const std::vector<double>& hist1, const std::vector<double>& hist2) {
    double distance = 0.0;
    for (size_t i = 0; i < hist1.size(); ++i) {
        double sum_h = hist1[i] + hist2[i];
        if (sum_h > 1e-9) { // Avoid division by zero if both bins are empty
            distance += ((hist1[i] - hist2[i]) * (hist1[i] - hist2[i])) / sum_h;
        }
    }
    return distance / 2.0; // Some definitions include /2, some don't. Let's include it.
}

} // end anonymous namespace

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

template<>
double ComputeError<Metric::SSIM>(const Image& img1, const Image& img2, float* error_map)
{
    if (img1.GetWidth() != img2.GetWidth() || img1.GetHeight() != img2.GetHeight()) {
        // Or handle error appropriately
        fprintf(stderr, "SSIM: Image dimensions must match.\n");
        return -1.0; // Or some error indicator
    }

    const int width = img1.GetWidth();
    const int height = img1.GetHeight();
    const float* data1 = img1.GetData();
    const float* data2 = img2.GetData();
    constexpr int num_channels = 4; // As defined in Image class, though we use luminance

    const int win_size = 8; // Default window size for SSIM
    const double K1 = 0.01, K2 = 0.03;
    const double L = 1.0; // Max pixel value (dynamic range, images are normalized to 0-1)
    const double C1 = (K1 * L) * (K1 * L);
    const double C2 = (K2 * L) * (K2 * L);

    double total_ssim = 0.0;
    int num_windows = 0;

    // Iterate over image by windows
    // Step can be win_size for non-overlapping, or 1 for fully overlapping (slower)
    // For simplicity, using non-overlapping windows here.
    // A common approach is to use a Gaussian weighting for the window, this is a simplified version.
    for (int y = 0; y <= height - win_size; y += win_size) {
        for (int x = 0; x <= width - win_size; x += win_size) {
            double mu1 = calculate_mean(data1, width, height, x, y, win_size, num_channels);
            double mu2 = calculate_mean(data2, width, height, x, y, win_size, num_channels);

            double sigma1_sq = calculate_variance(data1, width, height, x, y, win_size, num_channels, mu1);
            double sigma2_sq = calculate_variance(data2, width, height, x, y, win_size, num_channels, mu2);
            double sigma12 = calculate_covariance(data1, data2, width, height, x, y, win_size, num_channels, mu1, mu2);

            double ssim_numerator = (2 * mu1 * mu2 + C1) * (2 * sigma12 + C2);
            double ssim_denominator = (mu1 * mu1 + mu2 * mu2 + C1) * (sigma1_sq + sigma2_sq + C2);
            double ssim_val = ssim_numerator / ssim_denominator;

            total_ssim += ssim_val;
            num_windows++;

            if (error_map) { // Store local SSIM in error_map (average over window)
                for (int r = 0; r < win_size; ++r) {
                    for (int c = 0; c < win_size; ++c) {
                        int current_y = y + r;
                        int current_x = x + c;
                         // Check boundary, though window iteration should be within bounds
                        if (current_x < width && current_y < height) {
                            error_map[current_y * width + current_x] = static_cast<float>(ssim_val);
                        }
                    }
                }
            }
        }
    }

    if (num_windows == 0) return 0.0; // Or handle as error if images too small for any window
    return total_ssim / num_windows;
}

template<>
double ComputeError<Metric::ColorHistogramDistance>(const Image& img1, const Image& img2, float* error_map)
{
    if (img1.GetWidth() != img2.GetWidth() || img1.GetHeight() != img2.GetHeight()) {
        fprintf(stderr, "ColorHistogramDistance: Image dimensions must match for this implementation strategy (though fundamentally not required for histograms).\n");
        // For simplicity, we'll require same dimensions now.
        // A more general version could work even with different sizes.
        return -1.0; // Or some error indicator
    }
    if (img1.GetWidth() == 0 || img1.GetHeight() == 0) {
         return 0; // Or handle error
    }

    const int width = img1.GetWidth();
    const int height = img1.GetHeight();
    const float* data1 = img1.GetData();
    const float* data2 = img2.GetData();
    constexpr int num_channels = 4; // RGBA

    std::vector<double> hist1_r, hist1_g, hist1_b;
    std::vector<double> hist2_r, hist2_g, hist2_b;

    calculate_histogram_channel(data1, width, height, num_channels, 0, hist1_r); // R
    calculate_histogram_channel(data1, width, height, num_channels, 1, hist1_g); // G
    calculate_histogram_channel(data1, width, height, num_channels, 2, hist1_b); // B

    calculate_histogram_channel(data2, width, height, num_channels, 0, hist2_r); // R
    calculate_histogram_channel(data2, width, height, num_channels, 1, hist2_g); // G
    calculate_histogram_channel(data2, width, height, num_channels, 2, hist2_b); // B

    double dist_r = chi_squared_distance(hist1_r, hist2_r);
    double dist_g = chi_squared_distance(hist1_g, hist2_g);
    double dist_b = chi_squared_distance(hist1_b, hist2_b);

    // Combine distances (e.g., sum or average)
    // Summing them up. Max possible Chi-Squared is 1 per channel (if normalized and /2 not used) or 2 (if /2 used and one hist is empty where other is not).
    // With normalization and /2, it's typically between 0 and 1.
    double total_distance = dist_r + dist_g + dist_b;

    if (error_map) {
        // For global metrics, error_map is less meaningful per pixel.
        // Fill with the global distance value for all pixels.
        const size_t count = width * height;
        for (size_t i = 0; i < count; ++i) {
            error_map[i] = static_cast<float>(total_distance);
        }
    }
    return total_distance;
}

double Metric::CIEDE2000::operator()(const float* a_rgb, const float* b_rgb, size_t count) const {
    if (count < 3) return 0.0; // Expect at least R, G, B

    // Assuming a_rgb and b_rgb point to R, G, B channels
    // Image data is normalized [0,1].
    // For CIEDE2000, RGB should ideally be linear. Let's assume they are.
    // If they were sRGB, they'd need gamma expansion first.
    // E.g. R1_linear = (R1_srgb <= 0.04045) ? R1_srgb / 12.92 : pow((R1_srgb + 0.055)/1.055, 2.4)

    double x1, y1, z1, l1, lab_a1, lab_b1;
    rgb_to_xyz(a_rgb[0], a_rgb[1], a_rgb[2], x1, y1, z1);
    xyz_to_lab(x1, y1, z1, l1, lab_a1, lab_b1);

    double x2, y2, z2, l2, lab_a2, lab_b2;
    rgb_to_xyz(b_rgb[0], b_rgb[1], b_rgb[2], x2, y2, z2);
    xyz_to_lab(x2, y2, z2, l2, lab_a2, lab_b2);

    return ciede2000_delta_e(l1, lab_a1, lab_b1, l2, lab_a2, lab_b2);
}

template<>
double ComputeError<Metric::NCC>(const Image& img1, const Image& img2, float* error_map)
{
    if (img1.GetWidth() != img2.GetWidth() || img1.GetHeight() != img2.GetHeight()) {
        fprintf(stderr, "NCC: Image dimensions must match.\n");
        return -2.0; // NCC is typically [-1, 1]. Use -2 as an error indicator.
    }
    if (img1.GetWidth() == 0 || img1.GetHeight() == 0) {
         return 0; // Or handle error if images are empty
    }

    const int width = img1.GetWidth();
    const int height = img1.GetHeight();
    const size_t num_pixels = width * height;
    const float* data1 = img1.GetData();
    const float* data2 = img2.GetData();
    constexpr int num_channels = 4; // RGBA

    std::vector<float> gray1(num_pixels);
    std::vector<float> gray2(num_pixels);

    double sum_gray1 = 0.0;
    double sum_gray2 = 0.0;

    for (size_t i = 0; i < num_pixels; ++i) {
        gray1[i] = get_grayscale(data1 + i * num_channels);
        gray2[i] = get_grayscale(data2 + i * num_channels);
        sum_gray1 += gray1[i];
        sum_gray2 += gray2[i];
    }

    double mean_gray1 = sum_gray1 / num_pixels;
    double mean_gray2 = sum_gray2 / num_pixels;

    double numerator = 0.0;
    double sum_sq_diff1 = 0.0;
    double sum_sq_diff2 = 0.0;

    for (size_t i = 0; i < num_pixels; ++i) {
        double diff1 = gray1[i] - mean_gray1;
        double diff2 = gray2[i] - mean_gray2;
        numerator += diff1 * diff2;
        sum_sq_diff1 += diff1 * diff1;
        sum_sq_diff2 += diff2 * diff2;
    }

    double denominator = std::sqrt(sum_sq_diff1 * sum_sq_diff2);

    double ncc_result;
    if (denominator < 1e-9) { // Check for near zero denominator
        // If both sum_sq_diff1 and sum_sq_diff2 are zero, images are flat and identical in terms of diff from mean.
        // Consider this perfect correlation if means are also same, or undefined/0 otherwise.
        // A common practice: if denominator is zero, NCC is 1 if numerator is also zero (or positive), else 0 or specific value.
        // If numerator is also very small, effectively means both are constant fields.
        // If one image is constant (zero variance), NCC is typically 0 unless both are identical constants.
        if (std::abs(numerator) < 1e-9) { // Both effectively constant and same or numerator is zero.
             ncc_result = 1.0; // Or 0.0 if one is flat and other is not.
                               // If both are perfectly flat, numerator is 0, sum_sq_diffs are 0.
        } else {
             ncc_result = 0.0; // Cannot normalize, one or both signals have no variance.
        }
    } else {
        ncc_result = numerator / denominator;
    }

    // Ensure NCC is within [-1, 1] range due to potential floating point inaccuracies
    if (ncc_result > 1.0) ncc_result = 1.0;
    if (ncc_result < -1.0) ncc_result = -1.0;


    if (error_map) {
        for (size_t i = 0; i < num_pixels; ++i) {
            error_map[i] = static_cast<float>(ncc_result);
        }
    }
    return ncc_result;
}

template double ComputeError<Metric::MSE>(const Image& img1, const Image& img2, float* error_map);
template double ComputeError<Metric::RMSE>(const Image& img1, const Image& img2, float* error_map);
template double ComputeError<Metric::MAE>(const Image& img1, const Image& img2, float* error_map);
template double ComputeError<Metric::MAPE>(const Image& img1, const Image& img2, float* error_map);
template double ComputeError<Metric::PSNR>(const Image& img1, const Image& img2, float* error_map);
// Although ComputeError<Metric::SSIM> is specialized, this line is often kept
// to ensure the compiler knows this specific version is intended to be used/exported.
// Depending on compiler/linker, it might or might not be strictly necessary
// once the full specialization is defined in the .cpp.
// For consistency with other metrics, we can include it.
template double ComputeError<Metric::SSIM>(const Image& img1, const Image& img2, float* error_map);
template double ComputeError<Metric::ColorHistogramDistance>(const Image& img1, const Image& img2, float* error_map);
template double ComputeError<Metric::CIEDE2000>(const Image& img1, const Image& img2, float* error_map);
template double ComputeError<Metric::NCC>(const Image& img1, const Image& img2, float* error_map);
