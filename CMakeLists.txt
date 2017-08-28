# Copyright (c) 2017, David Hirvonen 
# All rights reserved.

cmake_minimum_required(VERSION 3.0)

### Hunter snapshot that will be used ###
include("cmake/HunterGate.cmake")
HunterGate(
  URL "https://github.com/ruslo/hunter/archive/v0.19.68.tar.gz"
  SHA1 "ad0d042d8ec56a3c6a4bda1e657f1f65b494ab12"
  LOCAL  
  )

option(DRISHTI_TEST_BUILD_MIN_SIZE "Toggle minsize (predict only) builds" OFF)

project(drishti-hunter-test)

###############
### drishti ###
###############

hunter_add_package(drishti)
find_package(drishti CONFIG REQUIRED)

hunter_add_package(cxxopts)
find_package(cxxopts CONFIG REQUIRED)

###########################
### drishti-hunter-test ###
###########################

add_executable(drishti-eye-test drishti-eye-test.cpp)
target_link_libraries(drishti-eye-test PUBLIC drishti::drishti)

install(TARGETS drishti-eye-test DESTINATION bin)
