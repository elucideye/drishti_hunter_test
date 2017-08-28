#include <drishti/EyeSegmenter.hpp>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <cxxopts.hpp>

int main(int argc, char **argv)
{
    const auto argumentCount = argc;
    
    std::string input, output, model;
    
    cxxopts::Options options("drishti-eye-test", "Command line interface for eye model fitting");
    
    // clang-format off
    options.add_options()
        // input/output:
        ("i,input", "Input image", cxxopts::value<std::string>(input))
        ("o,output", "Output image", cxxopts::value<std::string>(output))
        ("m,model", "Eye model (pose regression)", cxxopts::value<std::string>(model));
    // clang-format on

    options.parse(argc, argv);
    if ((argumentCount <= 1) || options.count("help"))
    {
        std::cout << options.help({ "" }) << std::endl;
        return 0;
    }
}

