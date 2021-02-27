#include <iostream>
#include <iomanip>

#include "argparse.hpp"

#include "error_metrics.h"

void PrintMetrics()
{
    std::cout << "Available error metrics:" << std::endl;
    for (auto& metric : g_error_metrics) {
        std::cout << "  " << std::left << std::setw(6) << metric.short_name << "(" << metric.long_name << ")" << std::endl;
    }
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
    program.add_argument("-t", "--threshold").default_value("0.0f").help("Error threshold");
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

    return 0;
}
