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
list(APPEND HUNTER_CACHE_SERVERS "https://github.com/elucideye/hunter-cache")

set(WORKING_CONFIG "${CMAKE_CURRENT_LIST_DIR}/drishti-upload/config.cmake")

option(DRISHTI_UPLOAD_IGNORE_SUBMODULES "Ignore submodules in drishti-upload" YES)

include("cmake/HunterGate.cmake")
HunterGate(
  URL "https://github.com/ruslo/hunter/archive/v0.19.85.tar.gz"
  SHA1 "6b0a65f5bb49f09cce73d07d91af0a3e54f07835"
  LOCAL  
  )

option(DRISHTI_TEST_BUILD_MIN_SIZE "Toggle minsize (predict only) builds" OFF)
option(DRISHTI_TEST_BUILD_TESTS "Build cross platform tests" OFF)

project(drishti-hunter-test)

if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  set(CMAKE_POLICY_DEFAULT_CMP0069 NEW) # for 3rd parties added by add_subdirectory
  cmake_policy(SET CMP0069 NEW)
endif()

string(COMPARE EQUAL "${CMAKE_SYSTEM_NAME}" "Linux" is_linux)

# Add this OpenMP block for toolchain compatibility with drishti libraries
# if(NOT XCODE AND NOT MSVC) # We'll never have openmp on ios, and this test is really slow
#   find_package( OpenMP )
#   if(OPENMP_FOUND)
#     message("OPENMP FOUND")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
#     set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
#   endif()
# endif()

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

#hunter_add_package(spdlog) # for logging
#find_package(spdlog CONFIG REQUIRED)

set(base_deps
  drishti::drishti
  cxxopts::cxxopts
  ${OpenCV_LIBS}
#  spdlog::spdlog  
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

add_executable(drishti-face-test drishti-face-test.cpp FaceTrackerFactoryJson.h FaceTrackerFactoryJson.cpp)
target_link_libraries(drishti-face-test PUBLIC ${base_deps} nlohmann_json aglet::aglet ${boost_libs})
if(DRISHTI_TEST_BUILD_TESTS)
  target_compile_definitions(drishti-face-test DRISHTI_TEST_BUILD_TESTS=1)
endif()
install(TARGETS drishti-face-test DESTINATION bin)

message("DRISHTI_ASSETS_EYE_MODEL_REGRESSOR : ${DRISHTI_ASSETS_EYE_MODEL_REGRESSOR}")
