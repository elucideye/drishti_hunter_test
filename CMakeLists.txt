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

hunter_add_package(cxxopts) # for CLI
find_package(cxxopts CONFIG REQUIRED)

hunter_add_package(spdlog) # for logging
find_package(spdlog CONFIG REQUIRED)

hunter_add_package(OpenCV) # for image IO
find_package(OpenCV REQUIRED)

set(base_deps
  drishti::drishti
  spdlog::spdlog
  cxxopts::cxxopts
  ${OpenCV_LIBS}
  )

if(DRISHTI_TEST_BUILD_TESTS)

  hunter_add_packages(gauze) # for tests
  find_package(gauze CONFIG REQUIRED)

  list(APPEND base_deps gauze::gauze)

  enable_testing()
  
endif()

########################
### drishti-eye-test ###
########################

add_executable(drishti-eye-test drishti-eye-test.cpp)
target_link_libraries(drishti-eye-test PUBLIC ${base_deps})
if(DRISHTI_TEST_BUILD_TESTS)
  target_compile_definitions(drishti-eye-test DRISHTI_TEST_BUILD_TESTS=1)
endif()
install(TARGETS drishti-eye-test DESTINATION bin)

#########################
### drishti-face-test ###
#########################

hunter_add_package(nlohmann_json)
find_package(nlohmann_json CONFIG REQUIRED)

hunter_add_package(aglet)
find_package(aglet CONFIG REQUIRED)

hunter_add_package(Boost COMPONENTS system filesystem)
find_package(Boost CONFIG REQUIRED system filesystem)
set(boost_libs Boost::system Boost::filesystem)

add_executable(drishti-face-test drishti-face-test.cpp)
target_link_libraries(drishti-face-test PUBLIC ${base_deps} nlohmann_json aglet::aglet ${boost_libs})
if(DRISHTI_TEST_BUILD_TESTS)
  target_compile_definitions(drishti-face-test DRISHTI_TEST_BUILD_TESTS=1)
endif()
install(TARGETS drishti-face-test DESTINATION bin)

message("DRISHTI_ASSETS_EYE_MODEL_REGRESSOR : ${DRISHTI_ASSETS_EYE_MODEL_REGRESSOR}")
