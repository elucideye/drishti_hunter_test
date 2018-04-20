/*!
  @file   drishti-face-test.cpp
  @author David Hirvonen
  @brief  Test the drishti face tracking API.

  \copyright Copyright 2017 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include <drishti/FaceTracker.hpp>
#include <drishti/drishti_cv.hpp>

#include <spdlog/spdlog.h> // for portable loggin
#include <spdlog/fmt/ostr.h>

#include "FaceTrackerFactoryJson.h"

#include <opencv2/core.hpp> // for cv::Mat
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

// https://stackoverflow.com/a/1567703
class Line
{
    std::string data;
public:
    friend std::istream &operator>>(std::istream &is, Line &l)
    {
        std::getline(is, l.data);
        return is;
    }
    operator std::string() const { return data; }
};

using FaceResources = drishti::sdk::FaceTracker::Resources;

static std::shared_ptr<spdlog::logger> createLogger(const char *name);
static std::shared_ptr<drishti::sdk::FaceTracker> create(FaceResources &factory, const cv::Size& size, int orientation, float fx);

// See: https://github.com/elucideye/drishti/blob/master/src/lib/drishti/drishti/ut/test-FaceTracker.cpp
struct FaceTrackTest
{
    FaceTrackTest(std::shared_ptr<spdlog::logger> &logger, const std::string &sOutput)
        : m_logger(logger)
        , m_output(sOutput)
        , m_counter(0)
    {
        table =
        {
            this,
            triggerFunc,
            callbackFunc,
            allocatorFunc
        };
    }
    
    static void draw(cv::Mat &image, const drishti::sdk::Eye &eye)
    {
        auto eyelids = drishti::sdk::drishtiToCv(eye.getEyelids());
        auto crease = drishti::sdk::drishtiToCv(eye.getCrease());
        auto pupil = drishti::sdk::drishtiToCv(eye.getPupil());
        auto iris = drishti::sdk::drishtiToCv(eye.getIris());
        
        cv::ellipse(image, iris, {0,255,0}, 1, 8);
        cv::ellipse(image, pupil, {0,255,0}, 1, 8);
        for (const auto &p : eyelids)
        {
            cv::circle(image, p, 2, {0,255,0}, -1, 8);
        }
        for (const auto &p : crease)
        {
            cv::circle(image, p, 2, {0,255,0}, -1, 8);
        }
    }
    
    static void draw(cv::Mat &image, const drishti::sdk::Face &face)
    {
        auto landmarks = drishti::sdk::drishtiToCv(face.landmarks);
        for (const auto &p : landmarks)
        {
            cv::circle(image, p, 2, {0,255,0}, -1, 8);
        }
        
        for (const auto &e : face.eyes)
        {
            draw(image, e);
        }
    }
    
    void setSizeHint(const cv::Size &size)
    {
        m_size = size;
    }

    int callback(drishti::sdk::Array<drishti_face_tracker_result_t, 64>& results)
    {
        m_logger->info("callback: Received results");
        
        if(results.size() > 0)
        {
            const auto &r = results[0];
            {
                cv::Mat canvas;
                if(r.image.image.getRows() > 0 && r.image.image.getCols() > 0)
                {
                    // Use the actual image as a canvas if it was requested ...
                    canvas = drishti::sdk::drishtiToCv<drishti::sdk::Vec4b, cv::Vec4b>(r.image.image).clone();
                }
                else
                {
                    // ... otherwise we create an empty image for this
                    canvas = cv::Mat::zeros(m_size.height, m_size.width, CV_8UC3);
                }
                
                for (const auto &f : r.faceModels)
                {
                    draw(canvas, f);
                }
                
                std::stringstream ss;
                ss << m_output << "/frame_" << std::setw(4) << std::setfill('0') << m_counter++ << ".png";
                
                cv::imwrite(ss.str(), canvas);
            }
        }
        
        return 0;
    }

    // Here we would typically add some critiera required to trigger a full capture
    // capture event.  We would do this selectively so that full frames are not
    // retrieved at each stage of processing. For example, if we want to capture a s
    // selfie image, we might monitor face positions over time to ensure that the
    // subject is relatively centered in the image and that there is fairly low
    // frame-to-frame motion.
    drishti_request_t trigger(const drishti_face_tracker_result_t& faces, double timestamp)
    {
        m_logger->info("trigger: Received results at time {}}", timestamp);

        if(faces.faceModels.size())
        {
            // Here for formulate the actual request, see drishti_request_t:
            // 1) Retrieve the last N frames (1)
            // 2) Get frames in user memory (true)
            // 3) Get frames as texture ID's (true)
            // 4) Get full frame images (true)
            // 5) Get eye crop images (true)
            return { 1, true, true, true, true };
        }
        
        return { 0 };
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
    
    static drishti_request_t triggerFunc(void* context, const drishti_face_tracker_result_t& faces, double timestamp)
    {
        if (FaceTrackTest* ft = static_cast<FaceTrackTest*>(context))
        {
            return ft->trigger(faces, timestamp);
        }
        return { 0, false, false };
    }
    
    static int allocatorFunc(void* context, const drishti_image_t& spec, drishti::sdk::Image4b& image)
    {
        if (FaceTrackTest* ft = static_cast<FaceTrackTest*>(context))
        {
            return ft->allocator(spec, image);
        }
        return -1;
    }

    drishti_face_tracker_t table;

    std::shared_ptr<spdlog::logger> m_logger;
    std::string m_output;
    std::size_t m_counter;
    
    cv::Size m_size; // video resolution
};

int gauze_main(int argc, char **argv)
{
    const auto argumentCount = argc;

    std::string sInput, sOutput, sModels;

    float fx = 0.f;
    
    cxxopts::Options options("drishti-face-test", "Command line interface for face model fitting");
    
    // clang-format off
    options.add_options()
        // input/output:
        ("i,input", "Input image", cxxopts::value<std::string>(sInput))
        ("f,focal-length", "focal length", cxxopts::value<float>(fx))
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

    FaceTrackerFactoryJson factory(sModels, "drishti-face-test");

    // Create input file list (or single image)
    std::vector<std::string> lines = { sInput };
    if(sInput.find(".txt") != std::string::npos)
    {
        std::ifstream ifs(sInput);
        if(ifs)
        {
            lines.clear();
            std::copy(std::istream_iterator<Line>(ifs), std::istream_iterator<Line>(), std::back_inserter(lines));
        }
    }
    
    auto glContext = aglet::GLContext::create(aglet::GLContext::kAuto);
    if (!glContext)
    {
        logger->error("Failed to create OpenGL context");
        return 1;
    }
    
    (*glContext)();
    
    FaceTrackTest context(logger, sOutput);
    
    std::shared_ptr<drishti::sdk::FaceTracker> tracker;
    
    cv::Size size;
    for (const auto &filename : lines)
    {
        cv::Mat image = cv::imread(filename, cv::IMREAD_COLOR);
        if (image.empty())
        {
            logger->error("Unable to read image {}", sInput);
            continue;
        }
        if (image.channels() == 3)
        {
            cv::cvtColor(image, image, cv::COLOR_BGR2BGRA);
        }
        
        if (!tracker)
        {
            if(fx == 0.f)
            {
                fx = image.cols; // guess
            }
            tracker = create(factory.factory, image.size(), 0, fx);
            if (!tracker)
            {
                logger->error("Failed to create face tracker");
                return 1;
            }
            
            // Register callbacks:
            tracker->add(context.table);
            
            context.setSizeHint(image.size());
        }
        
        logger->info("Frame: {}", filename);
        
        if(size.area() && size != image.size())
        {
            logger->error("Frame dimensions must be consistent: {}{}", size.width, size.height);
            continue;
        }
        
        // Register callback:
        drishti::sdk::VideoFrame frame({ image.cols, image.rows }, image.ptr(), true, 0, DFLT_TEXTURE_FORMAT);
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
std::shared_ptr<drishti::sdk::FaceTracker> create(FaceResources &factory, const cv::Size& size, int orientation, float fx)
{
    drishti::sdk::Vec2f p(size.width/2, size.height/2);
    drishti::sdk::SensorModel::Intrinsic intrinsic(p, fx, {size.width, size.height});
    drishti::sdk::SensorModel::Extrinsic extrinsic(drishti::sdk::Matrix33f::eye());
    drishti::sdk::SensorModel sensor(intrinsic, extrinsic);

    drishti::sdk::Context context(sensor);
    context.setDoSingleFace(true);         // only detect 1 face per frame
    context.setMinDetectionDistance(0.f);  // min distance
    context.setMaxDetectionDistance(1.f);  // max distance
    context.setFaceFinderInterval(0.f);    // detect on every frame ...
    context.setAcfCalibration(0.001f);     // adjust detection sensitivity
    context.setRegressorCropScale(1.f);
    context.setMinTrackHits(1);
    context.setMaxTrackMisses(1);
    context.setMinFaceSeparation(1.f);
    context.setDoOptimizedPipeline(false); // no latency
    
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
