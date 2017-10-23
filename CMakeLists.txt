# Copyright (c) 2017, David Hirvonen 
# All rights reserved.

cmake_minimum_required(VERSION 3.3)

#########################
### CMAKE_MODULE_PATH ###
#########################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/drishti-upload/cmake/Modules")

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

if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  set(CMAKE_POLICY_DEFAULT_CMP0069 NEW) # for 3rd parties added by add_subdirectory
  cmake_policy(SET CMP0069 NEW)
endif()

string(COMPARE EQUAL "${CMAKE_SYSTEM_NAME}" "Linux" is_linux)

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

  hunter_add_packages(gauze) # for tests
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
