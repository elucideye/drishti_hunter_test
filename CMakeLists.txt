# Copyright (c) 2017, David Hirvonen
# All rights reserved.

cmake_minimum_required(VERSION 3.9)

############################
### HunterGate and cache ###
############################

# drishti specific hunter cache pacakges:
set(
    HUNTER_CACHE_SERVERS
    "https://github.com/elucideye/hunter-cache"
    CACHE
    STRING
    "Hunter cache servers"
)

set(WORKING_CONFIG "${CMAKE_CURRENT_LIST_DIR}/drishti-upload/config.cmake")
option(DRISHTI_UPLOAD_IGNORE_SUBMODULES "Ignore submodules in drishti-upload" YES)
option(DRISHTI_AS_SUBMODULE "Include drishti as a submodule" NO)

include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.19.131.tar.gz"
    SHA1 "bc0eec34712af8854085c2dc6a90b78df17a9743"
    LOCAL # Note config.cmake includes ${WORKING_CONFIG}
)

option(DRISHTI_TEST_BUILD_MIN_SIZE "Toggle minsize (predict only) builds" OFF)
option(DRISHTI_TEST_BUILD_TESTS "Build cross platform tests" OFF)

project(drishti-hunter-test)

###############
### drishti ###
###############

hunter_add_package(drishti)
find_package(drishti CONFIG REQUIRED)

############
### Data ###
############

hunter_add_package(drishti_assets)
find_package(drishti_assets CONFIG REQUIRED)

###############
### Utility ###
###############

hunter_add_package(OpenCV) # for image IO
find_package(OpenCV REQUIRED)

hunter_add_package(cxxopts) # for CLI
find_package(cxxopts CONFIG REQUIRED)

hunter_add_package(spdlog) # for logging
find_package(spdlog CONFIG REQUIRED)

set(base_deps
  drishti::drishti
  cxxopts::cxxopts
  ${OpenCV_LIBS}
  spdlog::spdlog
  )

if(DRISHTI_TEST_BUILD_TESTS)

  hunter_add_package(gauze) # for tests
  find_package(gauze CONFIG REQUIRED)

  list(APPEND base_deps gauze::gauze)

  enable_testing()

endif()

########################
#### std::to_string ####
########################

# Workaround for incomplete C++11 implementations (gcc android)
try_compile(DRISHTI_HUNTER_TEST_HAVE_TO_STRING
  "${CMAKE_BINARY_DIR}/compile_tests"
  "${PROJECT_SOURCE_DIR}/cmake/to_string.cpp"
  )
if(DRISHTI_HUNTER_TEST_HAVE_TO_STRING)
  set(DRISHTI_HUNTER_TEST_ADD_TO_STRING OFF)
else()
  set(DRISHTI_HUNTER_TEST_ADD_TO_STRING ON)
endif()

add_subdirectory(src)

message("DRISHTI_ASSETS_EYE_MODEL_REGRESSOR : ${DRISHTI_ASSETS_EYE_MODEL_REGRESSOR}")
