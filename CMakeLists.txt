# Copyright (c) 2017, David Hirvonen
# All rights reserved.

cmake_minimum_required(VERSION 3.9)

# By default, the source code for all managed dependencies will be removed after
# building and installing to the cache.  This behavior is consistent with most 
# installed libraries (i.e., /usr/local/lib/*), but it does prevent stepping 
# through the dependencies in a debugger in cases where a problem is not 
# contained within the drishti project sources.  In such cases, you can set 
# HUNTER_KEEP_PACKAGE_SOURCES=ON from the command line during the project 
# configuration and the source will be left for all packages when they are
# created.  This setting must be used before a package is installed -- it 
# won't be applied retroactively to installed packages.  In order to re-install
# a package with sources you can always remove the cache
# (i.e., rm -rf ${HOME}/.hunter) or, less drastically you can modify the 
# CONFIG-ID of an installed package to trigger the configuration and 
# installation steps.  This can be done by modifying the input CMAKE_ARGS 
# list in a hunter_config() call.  In the following example KEEP_SOURCES=1
# is  added to trigger a re-installation:
#
#   hunter_config(foo VERSION ${HUNTER_foo_VERSION} CMAKE_ARGS KEEP_SOURCES=1)
#
# The HUNTER_KEEP_PACKAGE_SOURCES development feature is described here:
#
# In order to support stepping through package sources you will also have to
# make sure that debug versions of the packages are installed.  This will
# happen by default, but will not happen if you specify a release only build
# using HUNTER_CONFIGURATION_TYPES=Release

# https://docs.hunter.sh/en/latest/reference/user-variables.html#hunter-keep-package-sources
option(HUNTER_KEEP_PACKAGE_SOURCES "Keep installed package sources for debugging (caveat...)" ON)

# configure the cache upload
# NOTE: This runs only during CI builds on travis and appveyor
include("${CMAKE_CURRENT_LIST_DIR}/cmake/upload.cmake")

#########################
### CMAKE_MODULE_PATH ###
#########################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")

############################
### HunterGate and cache ###
############################

include("cmake/HunterGate.cmake")
HunterGate(
  URL "https://github.com/ruslo/hunter/archive/v0.20.74.tar.gz"
  SHA1 "88e3c806f0ff640c0e3cbc32b340ae2c370b31f5"
  LOCAL
)

option(DRISHTI_TEST_BUILD_MIN_SIZE "Toggle minsize (predict only) builds" ON)
option(DRISHTI_TEST_BUILD_TESTS "Build cross platform tests" OFF)
option(DRISHTI_OPENGL_ES3 "Support OpenGL ES 3.0 (default 2.0)" OFF)

project(drishti-hunter-test)

######################
### RPATH defaults ###
######################

# see: http://www.cmake.org/Wiki/CMake_RPATH_handling
include(drishti_set_rpath)
drishti_set_rpath()

###############
### drishti ###
###############

if(DHT_USE_LOCAL_DRISHTI)
  option(DRISHTI_BUILD_OGLES_GPGPU "Build with OGLES_GPGPU" ON)
  add_subdirectory(src/3rdparty/drishti)
  add_library(drishti::drishti ALIAS drishti)
else()
  hunter_add_package(drishti)
  find_package(drishti CONFIG REQUIRED)
endif()

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

####################
#### localeconv ####
####################

# Workaround for incomplete C++11 implementations (gcc android)
try_compile(DRISHTI_HUNTER_TEST_HAVE_LOCALECONV
  "${CMAKE_BINARY_DIR}/compile_tests"
  "${PROJECT_SOURCE_DIR}/cmake/localeconv.cpp"
  )
if(DRISHTI_HUNTER_TEST_HAVE_LOCALECONV)
  set(DRISHTI_HUNTER_TEST_NO_LOCALECONV OFF)
else()
  set(DRISHTI_HUNTER_TEST_NO_LOCALECONV ON)
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
