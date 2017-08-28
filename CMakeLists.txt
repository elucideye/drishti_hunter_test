# Copyright (c) 2017, David Hirvonen 
# All rights reserved.

cmake_minimum_required(VERSION 3.0)

### Hunter snapshot that will be used ###
include("cmake/HunterGate.cmake")
HunterGate(
  URL "https://github.com/ruslo/hunter/archive/v0.19.84.tar.gz"
  SHA1 "9086d4b3c32784effdd6f83f45125b55cf95aae8"  
  LOCAL  
  )

option(DRISHTI_TEST_BUILD_MIN_SIZE "Toggle minsize (predict only) builds" OFF)

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

hunter_add_package(cxxopts) # for CLI
find_package(cxxopts CONFIG REQUIRED)

hunter_add_package(spdlog) # for logging
find_package(spdlog CONFIG REQUIRED)

hunter_add_package(OpenCV) # for image IO
find_package(OpenCV REQUIRED)

########################
### drishti-eye-test ###
########################

add_executable(drishti-eye-test drishti-eye-test.cpp)
target_link_libraries(drishti-eye-test PUBLIC drishti::drishti spdlog::spdlog cxxopts::cxxopts ${OpenCV_LIBS})
install(TARGETS drishti-eye-test DESTINATION bin)

#########################
### drishti-face-test ###
#########################

# drishti/Context.hpp:23:10: fatal error: 'drishti/sensor/Sensor.h' file not found

#add_executable(drishti-face-test drishti-face-test.cpp)
#target_link_libraries(drishti-face-test PUBLIC drishti::drishti spdlog::spdlog cxxopts::cxxopts ${OpenCV_LIBS})
#install(TARGETS drishti-face-test DESTINATION bin)

message("DRISHTI_ASSETS_EYE_MODEL_REGRESSOR : ${DRISHTI_ASSETS_EYE_MODEL_REGRESSOR}")
