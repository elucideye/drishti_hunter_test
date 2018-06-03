if(NOT DRISHTI_SDK_TEST_USE_LOCAL_DRISHTI)
  hunter_add_package(drishti)
  find_package(drishti CONFIG REQUIRED)
endif()

hunter_add_package(OpenCV) # for image IO
find_package(OpenCV REQUIRED)

hunter_add_package(cxxopts) # for CLI
find_package(cxxopts CONFIG REQUIRED)

hunter_add_package(spdlog) # for logging
find_package(spdlog CONFIG REQUIRED)

set(base_deps drishti::drishti cxxopts::cxxopts ${OpenCV_LIBS} spdlog::spdlog)

if(DRISHTI_SDK_TEST_BUILD_TESTS)
  hunter_add_package(gauze) # for tests
  find_package(gauze CONFIG REQUIRED)
  list(APPEND base_deps gauze::gauze)
endif()

add_subdirectory(eye)
add_subdirectory(face)
