/*!
  @file   drishti-face-test.cpp
  @author David Hirvonen
  @brief  Test the drishti face tracking API.

  \copyright Copyright 2017 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

// Need std:: extensions for android targets
#if defined(DRISHTI_HUNTER_TEST_ADD_TO_STRING)
#  include "stdlib_string.h"
#endif

#include <spdlog/spdlog.h> // for portable logging
#include <spdlog/fmt/ostr.h>

#include "FaceTrackerTest.h"
#include "FaceTrackerFactoryJson.h"
#include "VideoCaptureList.h"

#include <opencv2/core.hpp>    // for cv::Mat
#include <opencv2/imgproc.hpp> // for cv::cvtColor()
#include <opencv2/highgui.hpp> // for cv::imread()

#include <aglet/GLContext.h> // for portable opengl context

#include <cxxopts.hpp> // for CLI parsing

#include <fstream>
#include <istream>
#include <sstream>
#include <iomanip>

// clang-format off
#ifdef ANDROID
#  define DFLT_TEXTURE_FORMAT GL_RGBA
#else
#  define DFLT_TEXTURE_FORMAT GL_BGRA
#endif
// clang-format on

struct Params
{
    float focalLenth = 0.f;
    
    bool multiFace = false;
    float minDetectionDistance = 0.f;
    float maxDetectionDistance = 0.f;
    float faceFinderInterval = 0.f;
    float acfCalibration = 0.f;
    float regressorCropScale = 1.1f;
    int minTrackHits = 3;
    int maxTrackMisses = 2;
    float minFaceSeparation = 1.0f;
    bool doSimplePipeline = false;
    bool doAnnotation = false;
};

static void from_json(const std::string &filename, Params &params);
static void to_json(const std::string &filename, const Params &params);

using FaceResources = drishti::sdk::FaceTracker::Resources;
static std::shared_ptr<cv::VideoCapture> create(const std::string& filename);
static std::shared_ptr<spdlog::logger> createLogger(const char* name);
static cv::Size getSize(const cv::VideoCapture& video);

int gauze_main(int argc, char** argv)
{
    const auto argumentCount = argc;

    float captureZ = 0.f;
    bool doPreview = false;
    std::string sInput, sOutput, sModels, sConfig, sBoilerplate;
    
    Params params;
    
    cxxopts::Options options("drishti-face-test", "Command line interface for face model fitting");

    // clang-format off
    options.add_options()
        // input/output:
        ("i,input", "Input image", cxxopts::value<std::string>(sInput))
        ("o,output", "Output image", cxxopts::value<std::string>(sOutput))
        ("m,models", "Model factory configuration file (JSON)", cxxopts::value<std::string>(sModels))
        ("c,config", "Configuration file", cxxopts::value<std::string>(sConfig))
        ("boilerplate", "Dump boilerplate json file (then quit)", cxxopts::value<std::string>(sBoilerplate))
    
        // context parameters (configuratino):
        ("focal-length", "focal length", cxxopts::value<float>(params.focalLenth))
        ("multi-face", "Support multiple faces", cxxopts::value<bool>(params.multiFace))
        ("min", "Closest object distance", cxxopts::value<float>(params.minDetectionDistance))
        ("max", "Farthest object distance", cxxopts::value<float>(params.maxDetectionDistance))
        ("interval", "Face detection interval", cxxopts::value<float>(params.faceFinderInterval))
        ("calibration", "ACF detection calibration term", cxxopts::value<float>(params.acfCalibration))
        ("scale", "Regressor crop scale", cxxopts::value<float>(params.regressorCropScale))
        ("min-track-hits", "Min track hits (before init)", cxxopts::value<int>(params.minTrackHits))
        ("max-track-misses", "Max track misses (before terminatino)", cxxopts::value<int>(params.maxTrackMisses))
        ("separation", "Min face separations", cxxopts::value<float>(params.minFaceSeparation))
        ("simple", "Run the simple pipeline", cxxopts::value<bool>(params.doSimplePipeline))
        ("annotation", "Annotate the preview texture", cxxopts::value<bool>(params.doAnnotation))
    
        // behavior:
        ("capture", "Target capture distance", cxxopts::value<float>(captureZ))
        ("p,preview", "Preview window", cxxopts::value<bool>(doPreview))
    ;
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
    
    if(!sBoilerplate.empty())
    {
        to_json(sBoilerplate, params);
        return 0;
    }
    
    // User must specify either:
    // (a) json file with complete parameter set
    // (b) focal length + min/max distance
    if(!sConfig.empty())
    {
        from_json(sConfig, params);
    }
    else
    {
        if (options.count("focal-length") != 1)
        {
            logger->error("Must specify focal length (in pixels)");
            return 1;
        }
        
        if (options.count("min") != 1)
        {
            logger->error("Must specify closest object distance");
            return 1;
        }
        
        if (options.count("max") != 1)
        {
            logger->error("Must specify farthest object distance");
            return 1;
        }
    }

    if (params.minDetectionDistance > params.maxDetectionDistance)
    {
        logger->error("Search range requires minZ < maxZ");
        return 1;
    }

    FaceTrackerFactoryJson factory(sModels, "drishti-face-test");

    // :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    // Allocate a video source and get the video frame dimensions:
    // :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    auto video = create(sInput);
    if (!video)
    {
        logger->error("Failed to create video source for {}", sInput);
        return 1;
    }
    cv::Size size = getSize(*video);

    // :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    // Create an OpenGL context (w/ optional window):
    // :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    std::shared_ptr<aglet::GLContext> glContext;
    if (doPreview)
    {
        glContext = aglet::GLContext::create(aglet::GLContext::kAuto, "drishti-face-test", size.width, size.height);
    }
    else
    {
        glContext = aglet::GLContext::create(aglet::GLContext::kAuto);
    }

    if (!glContext)
    {
        logger->error("Failed to create OpenGL context");
        return 1;
    }

    (*glContext)();

    // :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    // Instantiate face tracking callbacks:
    // :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

    std::shared_ptr<drishti::sdk::FaceTracker> tracker;

    { // Configure the face tracker parameters:
        drishti::sdk::Vec2f p(size.width / 2, size.height / 2);
        drishti::sdk::SensorModel::Intrinsic intrinsic(p, params.focalLenth, { size.width, size.height });
        drishti::sdk::SensorModel::Extrinsic extrinsic(drishti::sdk::Matrix33f::eye());
        drishti::sdk::SensorModel sensor(intrinsic, extrinsic);

        drishti::sdk::Context context(sensor);
        context.setDoSingleFace(!params.multiFace);                    // only detect 1 face per frame
        context.setMinDetectionDistance(params.minDetectionDistance);  // min distance
        context.setMaxDetectionDistance(params.maxDetectionDistance);  // max distance
        context.setFaceFinderInterval(params.faceFinderInterval);      // detect on every frame ...
        context.setAcfCalibration(params.acfCalibration);              // adjust detection sensitivity
        context.setRegressorCropScale(params.regressorCropScale);      // regressor crop scale
        context.setMinTrackHits(params.minTrackHits);                  // # of hits before a new track is started
        context.setMaxTrackMisses(params.maxTrackMisses);              // # of misses before the track is abandoned
        context.setMinFaceSeparation(params.minFaceSeparation);        // min face separation
        context.setDoOptimizedPipeline(!params.doSimplePipeline);      // configure optimized pipeline
        context.setDoAnnotation(true);                                 // add default annotations for quick preview
        
        tracker = std::make_shared<drishti::sdk::FaceTracker>(&context, factory.factory);
        if (!tracker)
        {
            logger->error("Failed to create face tracker");
            return 1;
        }
    }

    // Register callbacks:
    FaceTrackTest callbacks(logger, sOutput);
    callbacks.setSizeHint(size);
    if (doPreview)
    {
        callbacks.initPreview(size, DFLT_TEXTURE_FORMAT);
    }

    if (options.count("capture") == 1)
    {
        // Set a capture volume on the camera's optical axis with a 1/3 meter radius
        // and be sure not to trigger a capture more than once every 8.0 seconds.
        callbacks.setCaptureSphere({ { 0.f, 0.f, captureZ } }, 0.33f, 8.0);
    }

    tracker->add(callbacks.table);

    float resolution = 1.0f;
    const auto tic = std::chrono::high_resolution_clock::now();
    std::size_t index = 0;
    
    // clang-format off
    std::function<bool()> process = [&]()
    {
        cv::Mat image;
        (*video) >> image;

        if (image.empty())
        {
            logger->error("Unable to read image {}", sInput);
            return false;
        }

        if (size.area() && (size != image.size()))
        {
            logger->error("Frame dimensions must be consistent: {}{}", size.width, size.height);
        }

        if (image.channels() == 3)
        {
            cv::cvtColor(image, image, cv::COLOR_BGR2BGRA);
        }

        if (doPreview)
        { // Update window properties (if used):
            auto& win = glContext->getGeometry();
            callbacks.setPreviewGeometry(win.tx, win.ty, win.sx * resolution, win.sy * resolution);
        }

        // Register callback:
        drishti::sdk::VideoFrame frame({ image.cols, image.rows }, image.ptr(), true, 0, DFLT_TEXTURE_FORMAT);
        (*tracker)(frame);

        { // Comnpute simple/global FPS
            const auto toc = std::chrono::high_resolution_clock::now();
            const double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(toc - tic).count();
            const double fps = static_cast<double>(index + 1) / elapsed;
            logger->info("Frame: {} fps = {}", index++, fps);
        }

        return true;
    };
    // clang-format on

    (*glContext)(process);

    return 0;
}

#if !defined(DRISHTI_TEST_BUILD_TESTS)
int main(int argc, char** argv)
{
    try
    {
        return gauze_main(argc, argv);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
#endif

static std::shared_ptr<spdlog::logger> createLogger(const char* name)
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

static std::shared_ptr<cv::VideoCapture> create(const std::string& filename)
{
    if (filename.find_first_not_of("0123456789") == std::string::npos)
    {
        return std::make_shared<cv::VideoCapture>(std::stoi(filename));
    }
    else if (filename.find(".txt") != std::string::npos)
    {
        return std::make_shared<VideoCaptureList>(filename);
    }
    else
    {
        return std::make_shared<cv::VideoCapture>(filename);
    }
}

static cv::Size getSize(const cv::VideoCapture& video)
{
    // clang-format off
    return
    {
        static_cast<int>(video.get(cv::CAP_PROP_FRAME_WIDTH)),
        static_cast<int>(video.get(cv::CAP_PROP_FRAME_HEIGHT))
    };
    // clang-format on
};

// Need std:: extensions for android targets
#if defined(DRISHTI_HUNTER_TEST_ADD_TO_STRING)
#include "stdlib_string.h"
#endif

#include <nlohmann/json.hpp> // nlohman-json

static void from_json(const nlohmann::json &json, Params &params)
{
    params.focalLenth = json.at("focalLength").get<float>();
    params.multiFace  = json.at("multiFace").get<bool>();
    params.minDetectionDistance = json.at("minDetectionDistance").get<float>();
    params.maxDetectionDistance = json.at("maxDetectionDistance").get<float>();
    params.faceFinderInterval = json.at("faceFinderInterval").get<float>();
    params.acfCalibration = json.at("acfCalibration").get<float>();
    params.regressorCropScale = json.at("regressorCropScale").get<int>();
    params.minTrackHits = json.at("minTrackHits").get<int>();
    params.maxTrackMisses = json.at("maxTrackMisses").get<int>();
    params.minFaceSeparation = json.at("minFaceSeparation").get<float>();
    params.doSimplePipeline = json.at("doSimplePipeline").get<bool>();
    params.doAnnotation = json.at("doAnnotation").get<bool>();
}

static void from_json(const std::string &filename, Params &params)
{
    std::ifstream ifs(filename);
    if (!ifs)
    {
        throw std::runtime_error("from_json() failed to open " + filename);
    }
    

    nlohmann::json json;
    ifs >> json;
    from_json(json, params);
}

static void to_json(nlohmann::json &json, const Params &params)
{
    json = nlohmann::json
    {
        {"focalLength", params.focalLenth},
        {"multiFace", params.multiFace},
        {"minDetectionDistance", params.minDetectionDistance},
        {"maxDetectionDistance", params.maxDetectionDistance},
        {"faceFinderInterval", params.faceFinderInterval},
        {"acfCalibration", params.acfCalibration},
        {"regressorCropScale", params.regressorCropScale},
        {"minTrackHits", params.minTrackHits},
        {"maxTrackMisses", params.maxTrackMisses},
        {"minFaceSeparation", params.minFaceSeparation},
        {"doSimplePipeline", params.doSimplePipeline},
        {"doAnnotation", params.doAnnotation}
    };
}

static void to_json(const std::string &filename, const Params &params)
{
    std::ofstream ofs(filename);
    if (!ofs)
    {
        throw std::runtime_error("to_json() failed to open " + filename);
    }
    
    nlohmann::json json;
    to_json(json, params);
    ofs <<  std::setw(4) << json;
    
}
