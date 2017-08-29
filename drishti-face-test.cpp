#include <drishti/FaceTracker.hpp>

#include <opencv2/core.hpp> // for cv::Mat
#include <opencv2/imgproc.hpp> // for cv::cvtColor()
#include <opencv2/highgui.hpp> // for cv::imread()

#include <aglet/GLContext.h> // for portable opengl context

#include <cxxopts.hpp> // for CLI parsing

#include <spdlog/spdlog.h> // for portable loggin

// Need std:: extensions for android targets 
//#include <nlohmann/json.hpp> // nlohman-json
#include "nlohmann/json.hpp" // nlohman-json

#include <boost/filesystem.hpp> // for portable path (de)construction

#include <fstream>

// clang-format off
#ifdef ANDROID
#  define DFLT_TEXTURE_FORMAT GL_RGBA
#else
#  define DFLT_TEXTURE_FORMAT GL_BGRA
#endif
// clang-format on

using FaceResources = drishti::sdk::FaceTracker::Resources;

static std::shared_ptr<spdlog::logger> createLogger(const char *name);
static std::shared_ptr<drishti::sdk::FaceTracker> create(FaceResources &factory, const cv::Size& size, int orientation);
inline std::string cat(const std::string &a, const std::string &b)
{
    return a + b;
}

namespace bfs = boost::filesystem;

class FactoryLoader
{
public:
    FactoryLoader(const std::string &sModels, const std::string &logger)
    {
        std::ifstream ifs(sModels);
        if (!ifs)
        {
            throw std::runtime_error(cat("FactoryLoader::FactoryLoader() failed to open ", sModels));
        }

        nlohmann::json json;
        ifs >> json;
        
        factory.logger = logger; // logger name
        std::vector< std::pair<const char *, std::istream **> > bindings =
        {
            { "face_detector", &factory.sFaceDetector },
            { "eye_model_regressor", &factory.sEyeRegressor },
            { "face_landmark_regressor", &factory.sFaceRegressor },
            { "face_detector_mean", &factory.sFaceModel }
        };
        
        // Get the directory name:
        auto path = bfs::path(sModels);
        for(auto &binding : bindings)
        {
            auto filename = path.parent_path() / json[binding.first].get<std::string>();
            std::shared_ptr<std::istream> stream = std::make_shared<std::ifstream>(filename.string());
            if(!stream || !stream->good())
            {
                throw std::runtime_error(cat("FactoryLoader::FactoryLoader() failed to open ", binding.first));
            }
            
            (*binding.second) = stream.get();            
            streams.push_back(stream);
        }
        
        good = true;
    }
    
    operator bool() const { return good; }

    drishti::sdk::FaceTracker::Resources factory;
    
protected:

    bool good = false;
    std::vector<std::shared_ptr<std::istream>> streams; 
};

struct FaceTrackTest
{
    FaceTrackTest(std::shared_ptr<spdlog::logger> &logger) : m_logger(logger)
    {
        table =
        {
            this,
            triggerFunc,
            callbackFunc,
            allocatorFunc
        };
    }
    
    int callback(drishti::sdk::Array<drishti_face_tracker_result_t, 64>& results)
    {
        m_logger->info("callback: Received results");
        
        for (const auto& r : results)
        {
            for (const auto &f : r.faceModels)
            {
                const auto &eye0 = f.getEye(0);
                const auto &eye1 = f.getEye(1);
                
                std::cout << eye0.getIris().center[0] << " " << eye0.getIris().center[1] << std::endl;
                std::cout << eye1.getIris().center[0] << " " << eye1.getIris().center[1] << std::endl;
            }
            //std::stringstream ss;
            //ss << "/tmp/image_" << count++ << ".png";
            //cv::imwrite(ss.str(), drishti::sdk::drishtiToCv<drishti::sdk::Vec4b, cv::Vec4b>(r.image));
        }
        
        return 0;
    }
    
    int trigger(const drishti::sdk::Vec3f& point, double timestamp)
    {
        m_logger->info("trigger: Received results: {}, {}, {} {}", point[0], point[1], point[2], timestamp);
        return 1; // force trigger
    }
    
    int allocator(const drishti_image_t& spec, drishti::sdk::Image4b& image)
    {
        m_logger->info("allocator: {} {}", spec.width, spec.height);
        return 0;
    }
    
    static int callbackFunc(void* context, drishti::sdk::Array<drishti_face_tracker_result_t, 64>& results)
    {
        if (FaceTrackTest* ft = static_cast<FaceTrackTest*>(context))
        {
            return ft->callback(results);
        }
        return -1;
    }
    
    static int triggerFunc(void* context, const drishti::sdk::Vec3f& point, double timestamp)
    {
        if (FaceTrackTest* ft = static_cast<FaceTrackTest*>(context))
        {
            return ft->trigger(point, timestamp);
        }
        return -1;
    }
    
    static int allocatorFunc(void* context, const drishti_image_t& spec, drishti::sdk::Image4b& image)
    {
        if (FaceTrackTest* ft = static_cast<FaceTrackTest*>(context))
        {
            return ft->allocator(spec, image);
        }
        return -1;
    }
    
    void init()
    {
       
    }
    
    drishti_face_tracker_t table;
    
    std::shared_ptr<spdlog::logger> m_logger;
};

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

    FactoryLoader factory(sModels, "drishti-face-test");

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

    auto glContext = aglet::GLContext::create(aglet::GLContext::kAuto);
    if (!glContext)
    {
        logger->error("Failed to create OpenGL context");
        return 1;
    }
    
    (*glContext)();
    
    auto tracker = create(factory.factory, image.size(), 0);
    if (!tracker)
    {
        logger->error("Failed to create face tracker");
        return 1;
    }

    FaceTrackTest context(logger);
    
    tracker->add(context.table);
    
    // Register callback:    
    drishti::sdk::VideoFrame frame({ image.cols, image.rows }, image.ptr(), true, 0, DFLT_TEXTURE_FORMAT);
    
    const int iterations = 10;
    for (int i = 0; i < iterations; i++)
    {
        (*tracker)(frame);
    }

    return 0;
}

#if !defined(DRISHTI_TEST_BUILD_TESTS)
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
#endif

/*
 * FaceFinder configuration can be achieved by modifying the input
 * configurations stored as member variables in the test fixture prior
 * to calling create()
 * 
 * @param size  : size of input frames for detector
 * @orientation : orientation of input frames
 */
std::shared_ptr<drishti::sdk::FaceTracker> create(FaceResources &factory, const cv::Size& size, int orientation)
{
    const float fx = size.width; // guess
    drishti::sdk::Vec2f p(size.width/2, size.height/2);
    drishti::sdk::SensorModel::Intrinsic intrinsic(p, fx, {size.width, size.height});
    drishti::sdk::SensorModel::Extrinsic extrinsic(drishti::sdk::Matrix33f::eye());
    drishti::sdk::SensorModel sensor(intrinsic, extrinsic);
    drishti::sdk::Context context(sensor);
    return std::make_shared<drishti::sdk::FaceTracker>(&context, factory);
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
