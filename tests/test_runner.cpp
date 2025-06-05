#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <limits>
#include <cassert>
#include <iomanip> // For std::fixed and std::setprecision

// Headers from src directory (will be found via include_directories in CMake)
#include "image.h"
#include "error_metrics.h"

// Helper to compare doubles with tolerance
bool AlmostEqual(double a, double b, double epsilon = 1e-5) {
    if (std::isinf(a) && std::isinf(b) && ((a > 0 && b > 0) || (a < 0 && b < 0)) ) return true;
    if (std::isnan(a) && std::isnan(b)) return true;
    // Check for one being inf and other not, or different signs of infinity
    if ((std::isinf(a) && !std::isinf(b)) || (!std::isinf(a) && std::isinf(b))) return false;
    if (std::isinf(a) && std::isinf(b) && ((a > 0 && b < 0) || (a < 0 && b > 0))) return false;

    return std::abs(a - b) < epsilon;
}

// Helper to create a solid color image
Image CreateSolidImage(uint32_t width, uint32_t height, float r, float g, float b, float alpha_val = 1.0) {
    Image img = Image::Create(width, height);
    float* data = img.GetData();
    for (uint32_t i = 0; i < width * height; ++i) {
        data[0] = r;
        data[1] = g;
        data[2] = b;
        data[3] = alpha_val;
        data += 4;
    }
    return img;
}

// Helper to create a simple gradient image (grayscale)
Image CreateGradientImage(uint32_t width, uint32_t height) {
    Image img = Image::Create(width, height);
    float* data = img.GetData();
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            float val = static_cast<float>(x + y) / static_cast<float>(width + height - 2); // Simple gradient 0-1
            val = val > 1.0f ? 1.0f : (val < 0.0f ? 0.0f : val);
            data[0] = val; // R
            data[1] = val; // G
            data[2] = val; // B
            data[3] = 1.0f; // A
            data += 4;
        }
    }
    return img;
}

// Test function for PSNR
bool TestPsnr(const std::vector<ErrorMetric>& metrics) {
    // std::cout << "Testing PSNR..." << std::endl; // Moved to main
    bool success = true;

    Image img_black_2x2 = CreateSolidImage(2, 2, 0.0f, 0.0f, 0.0f);
    Image img_white_2x2 = CreateSolidImage(2, 2, 1.0f, 1.0f, 1.0f);
    Image img_gray_2x2  = CreateSolidImage(2, 2, 0.5f, 0.5f, 0.5f);

    const ErrorMetric* psnr_metric_ptr = nullptr;
    for(const auto& m : metrics) {
        if (m.short_name == "psnr") {
            psnr_metric_ptr = &m;
            break;
        }
    }
    assert(psnr_metric_ptr != nullptr && "PSNR metric not found in g_error_metrics");
    const ErrorMetric& psnr_metric = *psnr_metric_ptr;

    double psnr_identical = psnr_metric.ComputeError(img_black_2x2, img_black_2x2, nullptr);
    std::cout << "  PSNR (identical): " << psnr_identical << std::endl;
    if (!AlmostEqual(psnr_identical, std::numeric_limits<double>::infinity())) {
        std::cerr << "    FAIL: PSNR for identical images should be infinity. Got: " << psnr_identical << std::endl;
        success = false;
    }

    double psnr_black_white = psnr_metric.ComputeError(img_black_2x2, img_white_2x2, nullptr);
    double expected_psnr_black_white = -10.0 * std::log10((1.0*1.0*3.0)/4.0);
    std::cout << "  PSNR (black vs white): " << psnr_black_white << " (Expected: " << expected_psnr_black_white << ")" << std::endl;
    if (!AlmostEqual(psnr_black_white, expected_psnr_black_white, 1e-4)) {
        std::cerr << "    FAIL: PSNR for black vs white should be approx " << expected_psnr_black_white << ". Got: " << psnr_black_white << std::endl;
        success = false;
    }

    double psnr_black_gray = psnr_metric.ComputeError(img_black_2x2, img_gray_2x2, nullptr);
    double expected_psnr_black_gray = -10.0 * std::log10((0.5*0.5*3.0)/4.0);
    std::cout << "  PSNR (black vs gray): " << psnr_black_gray << " (Expected: " << expected_psnr_black_gray << ")" << std::endl;
    if (!AlmostEqual(psnr_black_gray, expected_psnr_black_gray, 1e-4)) {
        std::cerr << "    FAIL: PSNR for black vs gray should be approx " << expected_psnr_black_gray << ". Got: " << psnr_black_gray << std::endl;
        success = false;
    }

    std::cout << (success ? "  PSNR Tests PASSED." : "  PSNR Tests FAILED.") << std::endl << std::endl;
    return success;
}

bool TestSsim(const std::vector<ErrorMetric>& metrics) {
    // std::cout << "Testing SSIM..." << std::endl; // Moved to main
    bool success = true;

    Image img_black_8x8 = CreateSolidImage(8, 8, 0.0f, 0.0f, 0.0f);
    Image img_white_8x8 = CreateSolidImage(8, 8, 1.0f, 1.0f, 1.0f);
    Image img_grad_8x8 = CreateGradientImage(8, 8);

    const ErrorMetric* ssim_metric_ptr = nullptr;
    for(const auto& m : metrics) {
        if (m.short_name == "ssim") {
            ssim_metric_ptr = &m;
            break;
        }
    }
    assert(ssim_metric_ptr != nullptr && "SSIM metric not found");
    const ErrorMetric& ssim_metric = *ssim_metric_ptr;

    double ssim_identical = ssim_metric.ComputeError(img_black_8x8, img_black_8x8, nullptr);
    std::cout << "  SSIM (identical black): " << ssim_identical << std::endl;
    if (!AlmostEqual(ssim_identical, 1.0)) {
        std::cerr << "    FAIL: SSIM for identical black images should be 1.0. Got: " << ssim_identical << std::endl;
        success = false;
    }

    double ssim_identical_grad = ssim_metric.ComputeError(img_grad_8x8, img_grad_8x8, nullptr);
    std::cout << "  SSIM (identical gradient): " << ssim_identical_grad << std::endl;
    if (!AlmostEqual(ssim_identical_grad, 1.0)) {
        std::cerr << "    FAIL: SSIM for identical gradient images should be 1.0. Got: " << ssim_identical_grad << std::endl;
        success = false;
    }

    double ssim_black_white = ssim_metric.ComputeError(img_black_8x8, img_white_8x8, nullptr);
    double K1 = 0.01, L_range = 1.0;
    double C1 = (K1 * L_range) * (K1 * L_range);
    double expected_ssim_black_white = C1 / (1.0 + C1);
    std::cout << "  SSIM (black vs white): " << ssim_black_white << " (Expected: " << expected_ssim_black_white << ")" << std::endl;
    if (!AlmostEqual(ssim_black_white, expected_ssim_black_white, 1e-6)) {
        std::cerr << "    FAIL: SSIM for black vs white should be approx " << expected_ssim_black_white << ". Got: " << ssim_black_white << std::endl;
        success = false;
    }

    Image img_black_2x2 = CreateSolidImage(2, 2, 0.0f, 0.0f, 0.0f);
    double ssim_too_small = ssim_metric.ComputeError(img_black_2x2, img_black_2x2, nullptr);
    std::cout << "  SSIM (2x2 identical): " << ssim_too_small << std::endl;
     if (!AlmostEqual(ssim_too_small, 0.0)) {
        std::cerr << "    FAIL: SSIM for 2x2 image (too small for 8x8 window) should be 0.0. Got: " << ssim_too_small << std::endl;
        success = false;
    }

    std::cout << (success ? "  SSIM Tests PASSED." : "  SSIM Tests FAILED.") << std::endl << std::endl;
    return success;
}

bool TestHistDist(const std::vector<ErrorMetric>& metrics) {
    // std::cout << "Testing Color Histogram Distance..." << std::endl; // Moved to main
    bool success = true;

    Image img_black_2x2 = CreateSolidImage(2, 2, 0.0f, 0.0f, 0.0f);
    Image img_white_2x2 = CreateSolidImage(2, 2, 1.0f, 1.0f, 1.0f);
    Image img_red_2x2   = CreateSolidImage(2, 2, 1.0f, 0.0f, 0.0f);

    const ErrorMetric* hist_metric_ptr = nullptr;
    for(const auto& m : metrics) {
        if (m.short_name == "histdist") {
            hist_metric_ptr = &m;
            break;
        }
    }
    assert(hist_metric_ptr != nullptr && "HistDist metric not found");
    const ErrorMetric& hist_metric = *hist_metric_ptr;

    double hist_identical = hist_metric.ComputeError(img_black_2x2, img_black_2x2, nullptr);
    std::cout << "  HistDist (identical): " << hist_identical << std::endl;
    if (!AlmostEqual(hist_identical, 0.0)) {
        std::cerr << "    FAIL: HistDist for identical images should be 0.0. Got: " << hist_identical << std::endl;
        success = false;
    }

    double hist_black_white = hist_metric.ComputeError(img_black_2x2, img_white_2x2, nullptr);
    std::cout << "  HistDist (black vs white): " << hist_black_white << std::endl;
    if (!AlmostEqual(hist_black_white, 3.0)) {
        std::cerr << "    FAIL: HistDist for black vs white should be 3.0. Got: " << hist_black_white << std::endl;
        success = false;
    }

    double hist_black_red = hist_metric.ComputeError(img_black_2x2, img_red_2x2, nullptr);
    std::cout << "  HistDist (black vs red): " << hist_black_red << std::endl;
    if (!AlmostEqual(hist_black_red, 1.0)) {
        std::cerr << "    FAIL: HistDist for black vs red should be 1.0. Got: " << hist_black_red << std::endl;
        success = false;
    }

    std::cout << (success ? "  HistDist Tests PASSED." : "  HistDist Tests FAILED.") << std::endl << std::endl;
    return success;
}

bool TestCiede2000(const std::vector<ErrorMetric>& metrics) {
    // std::cout << "Testing CIEDE2000..." << std::endl; // Moved to main
    bool success = true;

    Image img_black_2x2 = CreateSolidImage(2, 2, 0.0f, 0.0f, 0.0f);
    Image img_white_2x2 = CreateSolidImage(2, 2, 1.0f, 1.0f, 1.0f);

    const ErrorMetric* ciede_metric_ptr = nullptr;
    for(const auto& m : metrics) {
        if (m.short_name == "ciede2000") {
            ciede_metric_ptr = &m;
            break;
        }
    }
    assert(ciede_metric_ptr != nullptr && "CIEDE2000 metric not found");
    const ErrorMetric& ciede_metric = *ciede_metric_ptr;

    double ciede_identical = ciede_metric.ComputeError(img_black_2x2, img_black_2x2, nullptr);
    std::cout << "  CIEDE2000 (identical): " << ciede_identical << std::endl;
    if (!AlmostEqual(ciede_identical, 0.0, 1e-4)) {
        std::cerr << "    FAIL: CIEDE2000 for identical images should be 0.0. Got: " << ciede_identical << std::endl;
        success = false;
    }

    double ciede_black_white = ciede_metric.ComputeError(img_black_2x2, img_white_2x2, nullptr);
    std::cout << "  CIEDE2000 (black vs white): " << ciede_black_white << std::endl;
    if (!AlmostEqual(ciede_black_white, 100.0, 1e-1)) {
        std::cerr << "    FAIL: CIEDE2000 for black vs white should be 100.0. Got: " << ciede_black_white << std::endl;
        success = false;
    }

    std::cout << (success ? "  CIEDE2000 Tests PASSED." : "  CIEDE2000 Tests FAILED.") << std::endl << std::endl;
    return success;
}

bool TestNcc(const std::vector<ErrorMetric>& metrics) {
    // std::cout << "Testing NCC..." << std::endl; // Moved to main
    bool success = true;

    Image img_black_2x2 = CreateSolidImage(2, 2, 0.0f, 0.0f, 0.0f);
    Image img_white_2x2 = CreateSolidImage(2, 2, 1.0f, 1.0f, 1.0f);
    Image img_gray_2x2  = CreateSolidImage(2, 2, 0.5f, 0.5f, 0.5f);
    Image img_grad_2x2  = CreateGradientImage(2, 2);

    const ErrorMetric* ncc_metric_ptr = nullptr;
    for(const auto& m : metrics) {
        if (m.short_name == "ncc") {
            ncc_metric_ptr = &m;
            break;
        }
    }
    assert(ncc_metric_ptr != nullptr && "NCC metric not found");
    const ErrorMetric& ncc_metric = *ncc_metric_ptr;

    double ncc_identical_flat = ncc_metric.ComputeError(img_black_2x2, img_black_2x2, nullptr);
    std::cout << "  NCC (identical flat black): " << ncc_identical_flat << std::endl;
    if (!AlmostEqual(ncc_identical_flat, 1.0)) {
        std::cerr << "    FAIL: NCC for identical flat black images should be 1.0. Got: " << ncc_identical_flat << std::endl;
        success = false;
    }

    double ncc_identical_grad = ncc_metric.ComputeError(img_grad_2x2, img_grad_2x2, nullptr);
    std::cout << "  NCC (identical gradient): " << ncc_identical_grad << std::endl;
    if (!AlmostEqual(ncc_identical_grad, 1.0, 1e-4)) {
        std::cerr << "    FAIL: NCC for identical gradient images should be 1.0. Got: " << ncc_identical_grad << std::endl;
        success = false;
    }

    double ncc_black_white = ncc_metric.ComputeError(img_black_2x2, img_white_2x2, nullptr);
    std::cout << "  NCC (black vs white): " << ncc_black_white << std::endl;
    if (!AlmostEqual(ncc_black_white, 1.0)) {
        std::cerr << "    FAIL: NCC for black vs white (both flat) should be 1.0 by current logic. Got: " << ncc_black_white << std::endl;
        success = false;
    }

    double ncc_black_gray = ncc_metric.ComputeError(img_black_2x2, img_gray_2x2, nullptr);
    std::cout << "  NCC (black vs gray): " << ncc_black_gray << std::endl;
    if (!AlmostEqual(ncc_black_gray, 1.0)) {
        std::cerr << "    FAIL: NCC for black vs gray (both flat) should be 1.0 by current logic. Got: " << ncc_black_gray << std::endl;
        success = false;
    }

    double ncc_grad_black = ncc_metric.ComputeError(img_grad_2x2, img_black_2x2, nullptr);
    std::cout << "  NCC (gradient vs black): " << ncc_grad_black << std::endl;
    if (!AlmostEqual(ncc_grad_black, 1.0)) {
        std::cerr << "    FAIL: NCC for gradient vs black should be 1.0 by current logic for zero variance. Got: " << ncc_grad_black << std::endl;
        success = false;
    }

    std::cout << (success ? "  NCC Tests PASSED." : "  NCC Tests FAILED.") << std::endl << std::endl;
    return success;
}

int main() {
    std::cout << std::fixed << std::setprecision(6);
    bool all_tests_passed = true;

    std::cout << "--- Running PSNR Tests ---" << std::endl;
    if (!TestPsnr(g_error_metrics)) all_tests_passed = false;

    std::cout << "--- Running SSIM Tests ---" << std::endl;
    if (!TestSsim(g_error_metrics)) all_tests_passed = false;

    std::cout << "--- Running HistDist Tests ---" << std::endl;
    if (!TestHistDist(g_error_metrics)) all_tests_passed = false;

    std::cout << "--- Running CIEDE2000 Tests ---" << std::endl;
    if (!TestCiede2000(g_error_metrics)) all_tests_passed = false;

    std::cout << "--- Running NCC Tests ---" << std::endl;
    if (!TestNcc(g_error_metrics)) all_tests_passed = false;

    std::cout << "-------------------------" << std::endl;
    if (all_tests_passed) {
        std::cout << "Overall Test Status: ALL TESTS PASSED!" << std::endl;
        return 0;
    } else {
        std::cerr << "Overall Test Status: AT LEAST ONE TEST FAILED!" << std::endl;
        return 1;
    }
}
