#include <drishti/FaceTracker.hpp>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <cxxopts.hpp>

#include <spdlog/spdlog.h>

// Need std:: extensions for android targets 
#include <nlohmann/json.hpp> // nlohman-json

#include <fstream>

// clang-format off
#ifdef ANDROID
#  define DFLT_TEXTURE_FORMAT GL_RGBA
#else
#  define DFLT_TEXTURE_FORMAT GL_BGRA
#endif
// clang-format on

static std::shared_ptr<spdlog::logger> createLogger(const char *name);
static std::shared_ptr<drishti::sdk::FaceTracker> create(const cv::Size& size, int orientation, bool doThreads);

class FactoryLoader
{
public:
    FactoryLoader(nlohmann::json &json, const std::string &name)
    {
        factory.logger = name;        
        std::vector< std::pair<const char *, std::istream **> > bindings =
        {
            { "face_detector", &factory.sFaceDetector },
            { "eye_model_regressor", &factory.sEyeRegressor },
            { "face_landmark_regressor", &factory.sFaceRegressor },
            { "face_detector_mean", &factory.sFaceModel }
        };

        for(auto &binding : bindings)
        {
            std::shared_ptr<std::istream> stream = std::make_shared<std::ifstream>(binding.first);
            if(!stream || !stream->good())
            {
                throw std::runtime_error("FactoryLoader::FactoryLoader() failed to open " + bindin.first);
            }
            
            (*binding.second) = stream.get();            
            streams.push_back(stream);
        }
    }        

    drishti::sdk::FaceTracker::Resources factory;

protected:
    
    std::vector<std::shared_ptr<std::istream>> streams; 
}   

int gauze_main(int argc, char **argv)
{
    const auto argumentCount = argc;

    std::string sInput, sOutput, sModels;
    
    cxxopts::Options options("drishti-face-test", "Command line interface for face model fitting");
    
    // clang-format off
    options.add_options()
        // input/output:
        ("i,input", "Input image", cxxopts::value<std::string>(sInput))
        ("o,output", "Output image", cxxopts::value<std::string>(sOutput))
        ("m,models", "Model factory configuration file (JSON)", cxxopts::value<std::string>(sModels));
    // clang-format on    

    options.parse(argc, argv);
    if ((argumentCount <= 1) || options.count("help"))
    {
        std::cout << options.help({ "" }) << std::endl;
        return 0;
    }

    auto logger = createLogger("drishti-face-test");
    
    if (sInput.empty())
    {
        logger->error("Must specify input {}", sInput);
        return 1;
    }

    if (sOutput.empty())
    {
        logger->error("Must specify output {}", sOutput);
        return 1;
    }

    if (sModels.empty())
    {
        logger->error("Must specify models file {}", sModels);
        return 1;
    }

    std::ifstream ifs(sModels);
    if (!ifs)
    {
        logger->error("Unable to open file {}", sModels);
    }
    nlohmann::json json(ifs);
    FactoryLoader factory(json, "drishti-face-test");    

    cv::Mat image = cv::imread(sInput, cv::IMREAD_COLOR);
    if (image.empty())
    {
        logger->error("Unable to read image {}", sInput);
        return 1;
    }

    if (image.channels() == 3)
    {
        cv::cvtColor(image, image, cv::COLOR_BGR2BGRA);
    }

    auto glContex = aglet::GLContext::create(aglet::GLContext::kAuto);
    if (!glContext)
    {
        logger->error("Failed to create OpenGL context");
        return 1;
    }
    
    auto tracker = create(factory.factory, image.size(), 0);
    if (!tracker)
    {
        logger->error("Failed to create face tracker");
        return 1;
    }

    drishti::sdk::VideoFrame frame({ image.cols, image.rows }, image.ptr(), true, 0, DFLT_TEXTURE_FORMAT);
    
    const int iterations = 10;
    for (int i = 0; i < iterations; i++)
    {
        (*tracker)(frame);
    }
}

int main(int argc, char **argv)
{
    try
    {
        return gauze_main(argc, argv);
    }
    catch(const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/*
 * FaceFinder configuration can be achieved by modifying the input
 * configurations stored as member variables in the test fixture prior
 * to calling create()
 * 
 * @param size  : size of input frames for detector
 * @orientation : orientation of input frames
 */
std::shared_ptr<drishti::sdk::FaceTracker> create(const FactoryLoader &factory, const cv::Size& size, int orientation)
{
    const float fx = size.width;
    const cv::Point2f p(image.cols / 2, image.rows / 2);
    drishti::sensor::SensorModel::Intrinsic params(p, fx, size);
    drishti::sensor::SensorModel sensor(params);

    drishti::sdk::Context context(sensor);
    auto tracker = std::make_shared<drishti::sdk::FaceTracker>(&context, factory);

    return tracker;
}

static std::shared_ptr<spdlog::logger> createLogger(const char *name)
{
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
#if defined(__ANDROID__)
    sinks.push_back(std::make_shared<spdlog::sinks::android_sink>());
#endif
    auto logger = std::make_shared<spdlog::logger>(name, begin(sinks), end(sinks));
    spdlog::register_logger(logger);
    spdlog::set_pattern("[%H:%M:%S.%e | thread:%t | %n | %l]: %v");
    return logger;
}
