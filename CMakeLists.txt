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
  URL "https://github.com/ruslo/hunter/archive/v0.21.9.tar.gz"
  SHA1 "0056188988906abb63a06c6f1aaef01832c62b74"
  LOCAL
  )

set(DRISHTI_SDK_TEST_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

option(DRISHTI_SDK_TEST_BUILD_MIN_SIZE "Toggle minsize (predict only) builds" ON)
option(DRISHTI_SDK_TEST_BUILD_TESTS "Build cross platform tests" OFF)
option(DRISHTI_SDK_TEST_OPENGL_ES3 "Support OpenGL ES 3.0 (default 2.0)" OFF)
option(DRISHTI_SDK_TEST_DRISHTI_BUILD_SHARED_SDK "Build drishti as a shared library" ON)

project(drishti-hunter-test VERSION 0.0.1)

######################
### RPATH defaults ###
######################

# see: http://www.cmake.org/Wiki/CMake_RPATH_handling
include(drishti_set_rpath)
drishti_set_rpath()

if(DRISHTI_SDK_TEST_BUILD_TESTS)
  enable_testing()
endif()

####################
#### localeconv ####
####################

# Workaround for incomplete C++11 implementations (gcc android)
try_compile(DRISHTI_SDK_TEST_HAVE_LOCALECONV
  "${CMAKE_BINARY_DIR}/compile_tests"
  "${PROJECT_SOURCE_DIR}/cmake/localeconv.cpp"
  )

########################
#### std::to_string ####
########################

# Workaround for incomplete C++11 implementations (gcc android)
try_compile(DRISHTI_SDK_TEST_HAVE_TO_STRING
  "${CMAKE_BINARY_DIR}/compile_tests"
  "${PROJECT_SOURCE_DIR}/cmake/to_string.cpp"
  )

add_subdirectory(src)

message("DRISHTI_ASSETS_EYE_MODEL_REGRESSOR : ${DRISHTI_ASSETS_EYE_MODEL_REGRESSOR}")
