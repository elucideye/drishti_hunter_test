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

using FaceResources = drishti::sdk::FaceTracker::Resources;
static std::shared_ptr<cv::VideoCapture> create(const std::string& filename);
static std::shared_ptr<spdlog::logger> createLogger(const char* name);
static cv::Size getSize(const cv::VideoCapture& video);

int gauze_main(int argc, char** argv)
{
    const auto argumentCount = argc;

    bool doPreview = false, doTurbo = false, doCapture = false;
    std::string sInput, sOutput, sModels;
    float fx = 0.f, minZ = 0.f, maxZ = 0.f, calibration = 0.0;

    cxxopts::Options options("drishti-face-test", "Command line interface for face model fitting");

    // clang-format off
    options.add_options()
        // input/output:
        ("i,input", "Input image", cxxopts::value<std::string>(sInput))
        ("f,focal-length", "focal length", cxxopts::value<float>(fx))
        ("min", "Closest object distance", cxxopts::value<float>(minZ))
        ("max", "Farthest object distance", cxxopts::value<float>(maxZ))
        ("c,calibration", "ACF detection calibration term", cxxopts::value<float>(calibration))
        ("o,output", "Output image", cxxopts::value<std::string>(sOutput))
        ("m,models", "Model factory configuration file (JSON)", cxxopts::value<std::string>(sModels))
        ("capture", "Perform capture every 8 seconds in central capture volume", cxxopts::value<bool>(doCapture))
        ("turbo", "Run the optimized pipeline (adds a little latency)", cxxopts::value<bool>(doTurbo))
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

    if (minZ > maxZ)
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
        drishti::sdk::SensorModel::Intrinsic intrinsic(p, fx, { size.width, size.height });
        drishti::sdk::SensorModel::Extrinsic extrinsic(drishti::sdk::Matrix33f::eye());
        drishti::sdk::SensorModel sensor(intrinsic, extrinsic);

        drishti::sdk::Context context(sensor);
        context.setDoSingleFace(true);          // only detect 1 face per frame
        context.setMinDetectionDistance(minZ);  // min distance
        context.setMaxDetectionDistance(maxZ);  // max distance
        context.setFaceFinderInterval(0.f);     // detect on every frame ...
        context.setAcfCalibration(calibration); // adjust detection sensitivity
        context.setRegressorCropScale(1.1f);
        context.setMinTrackHits(3);
        context.setMaxTrackMisses(2);
        context.setMinFaceSeparation(1.f);
        context.setDoOptimizedPipeline(doTurbo);
        context.setDoAnnotation(true); // add default annotations for quick preview

        tracker = std::make_shared<drishti::sdk::FaceTracker>(&context, factory.factory);
        if (!tracker)
        {
            logger->error("Failed to create face tracker");
            return 1;
        }
    }

    // Register callbacks:
    FaceTrackTest context(logger, sOutput);
    context.setSizeHint(size);
    if (doPreview)
    {
        context.initPreview(size, DFLT_TEXTURE_FORMAT);
    }

    if (doCapture)
    {
        // Set a capture volume on the camera's optical axis with a 1/3 meter radius
        // and be sure not to trigger a capture more than once every 8.0 seconds.
        context.setCaptureSphere({ { 0.f, 0.f, ((maxZ + minZ) / 2.0f) } }, 0.33f, 8.0);
    }

    tracker->add(context.table);

    float resolution = 1.0f;

    const auto tic = std::chrono::high_resolution_clock::now();

    std::size_t index = 0;
    std::function<bool()> process = [&]() {
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
            context.setPreviewGeometry(win.tx, win.ty, win.sx * resolution, win.sy * resolution);
        }

        // Register callback:
        drishti::sdk::VideoFrame frame({ image.cols, image.rows }, image.ptr(), true, 0, DFLT_TEXTURE_FORMAT);
        (*tracker)(frame);

        // Comnpute simple/global FPS
        const auto toc = std::chrono::high_resolution_clock::now();
        const double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(toc - tic).count();
        const double fps = static_cast<double>(index + 1) / elapsed;

        logger->info("Frame: {} fps = {}", index++, fps);

        return true;
    };

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
    return {
        static_cast<int>(video.get(cv::CAP_PROP_FRAME_WIDTH)),
        static_cast<int>(video.get(cv::CAP_PROP_FRAME_HEIGHT))
    };
};
