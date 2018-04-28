##############################################
# xgboost
##############################################

set(XGBOOST_CMAKE_ARGS
  XGBOOST_USE_HALF=ON
  XGBOOST_USE_CEREAL=ON
  )
if(DRISHTI_TEST_BUILD_MIN_SIZE)
  list(APPEND XGBOOST_CMAKE_ARGS XGBOOST_DO_LEAN=ON)
else()
  list(APPEND XGBOOST_CMAKE_ARGS XGBOOST_DO_LEAN=OFF)
endif()
hunter_config(xgboost VERSION 0.40-p10 CMAKE_ARGS ${XGBOOST_CMAKE_ARGS})

##############################################
# acf
##############################################

# note: currently acf depends on ogles_gpgpu OpenGL version
# -- there is no ACF_OPENGL_ES3
set(acf_cmake_args
  ACF_BUILD_TESTS=OFF 
  ACF_BUILD_EXAMPLES=OFF
  ACF_SERIALIZE_WITH_CVMATIO=OFF
  ACF_SERIALIZE_WITH_CEREAL=ON
  ACF_BUILD_OGLES_GPGPU=ON
  ACF_KEEPS_SOURCES=1
)
hunter_config(acf VERSION ${HUNTER_acf_VERSION} CMAKE_ARGS ${acf_cmake_args})

##############################################
# ogles_gpgpu
##############################################

if(APPLE AND NOT IOS) # temporary workaround on osx platform
  set(dht_ogles_gpgpu_submodule ON)
else()
  set(dht_ogles_gpgpu_submodule OFF)
endif()

option(DHT_OGLES_GPGPU_AS_SUBMODULE "Include ogles_gpgpu as a submodule" ${dht_ogles_gpgpu_submodule})

set(ogles_gpgpu_cmake_args
  OGLES_GPGPU_OPENGL_ES3=${DRISHTI_OPENGL_ES3}
)
if(DHT_OGLES_GPGPU_AS_SUBMODULE)
  if(NOT EXISTS "src/3rdparty/ogles_gpgpu")
    message(FATAL_ERROR "src/3rdparty/ogles_gpgpu submodule was requested, but does not exist")
  endif()
  hunter_config(ogles_gpgpu GIT_SUBMODULE "src/3rdparty/ogles_gpgpu" CMAKE_ARGS ${ogles_gpgpu_cmake_args})
else()
  hunter_config(ogles_gpgpu VERSION ${HUNTER_ogles_gpgpu_VERSION} CMAKE_ARGS ${ogles_gpgpu_cmake_args})
endif()

##############################################
# eigen
##############################################

set(eigen_cmake_args
  BUILD_TESTING=OFF
  HUNTER_INSTALL_LICENSE_FILES=COPYING.MPL2
  CMAKE_Fortran_COMPILER=OFF
)
hunter_config(Eigen VERSION ${HUNTER_Eigen_VERSION} CMAKE_ARGS ${eigen_cmake_args})

##############################################
# aglet
##############################################

set(aglet_cmake_args
  AGLET_OPENGL_ES3=${DRISHTI_OPENGL_ES3}
)
hunter_config(aglet VERSION ${HUNTER_aglet_VERSION} CMAKE_ARGS ${aglet_cmake_args})

##############################################
# drishti
##############################################

option(DHT_DRISHTI_AS_SUBMODULE "Include drishti as a submodule" ON)

set(drishti_cmake_args
  DRISHTI_BUILD_SHARED_SDK=OFF
  DRISHTI_OPENGL_ES3=${DRISHTI_OPENGL_ES3}
)
if(DHT_DRISHTI_AS_SUBMODULE)
  if(NOT EXISTS "src/3rdparty/drishti")
    message(FATAL_ERROR "src/3rdparty/drishti submodule was requested, but does not exist")
  endif()  
  hunter_config(drishti GIT_SUBMODULE "src/3rdparty/drishti" CMAKE_ARGS ${drishti_cmake_args})
else()
  hunter_config(drishti VERSION ${HUNTER_drishti_VERSION} CMAKE_ARGS ${drishti_cmake_args})
endif()

##############################################
# OpenCV
##############################################

string(COMPARE EQUAL "${CMAKE_OSX_SYSROOT}" "iphoneos" _is_ios)

if(_is_ios)
  set(_ios_args BUILD_WEBP=ON)
else()
  set(_ios_args "")
endif()

if(ANDROID)
  # This feature doesn't work with new CMake 3.7+ toolchains
  set(_android_args ENABLE_PRECOMPILED_HEADERS=OFF)
else()
  set(_android_args "")
endif()

set(opencv_cmake_args

  OPENCV_WITH_EXTRA_MODULES=OFF
  
  BUILD_ANDROID_EXAMPLES=OFF
  BUILD_DOCS=OFF
  BUILD_EXAMPLES=OFF
  BUILD_PERF_TESTS=OFF
  BUILD_TESTS=OFF
  BUILD_opencv_apps=OFF
  INSTALL_PYTHON_EXAMPLES=OFF
  BUILD_WITH_STATIC_CRT=OFF # Fix https://github.com/ruslo/hunter/issues/177
  ${_ios_args}
  ${_android_args}
  
  # Find packages in Hunter (instead of building from OpenCV sources)
  BUILD_ZLIB=OFF
  BUILD_TIFF=OFF
  BUILD_PNG=OFF
  BUILD_JPEG=OFF

  # include/jasper/jas_math.h:184:15: error: 'SIZE_MAX' undeclared (first use in this function)    
  BUILD_JASER=OFF
  WITH_JASPER=OFF
  
  # This stuff will build shared libraries. Build with PIC required for dependencies.
  BUILD_opencv_java=OFF
  BUILD_opencv_python2=OFF
  BUILD_opencv_python3=OFF
  # There is not a CUDA package so need to stop OpenCV from searching for it, otherwise
  #  it might pick up the host version
  WITH_CUDA=OFF
  WITH_CUFFT=OFF
  WITH_EIGEN=OFF

  WITH_PROTOBUF=OFF    # avoid protobuf errors
  BUILD_PROTOBUF=OFF   #  -/-
  BUILD_opencv_dnn=OFF #  -/-
  BUILD_LIBPROTOBUF_FROM_SOURCES=NO

  BUILD_opencv_imgproc=ON   # required for flow
  BUILD_opencv_optflow=ON   # optflow
  BUILD_opencv_plot=ON      # tracking -> plot    
  BUILD_opencv_tracking=ON  # tracking
  BUILD_opencv_video=ON     # tracking -> video
  BUILD_opencv_ximgproc=ON  # optflow -> ximgproc

  # Disable unused opencv_contrib modules (when OPENCV_WITH_EXTRA_MODULES == YES)
  BUILD_opencv_aruco=OFF
  BUILD_opencv_bgsegm=OFF
  BUILD_opencv_bioinspired=OFF
  BUILD_opencv_ccalib=OFF
  BUILD_opencv_cvv=OFF
  BUILD_opencv_datasets=OFF
  BUILD_opencv_dpm=OFF
  BUILD_opencv_face=OFF
  BUILD_opencv_fuzzy=OFF
  BUILD_opencv_hdf=OFF
  BUILD_opencv_line_descriptor=OFF
  BUILD_opencv_reg=OFF
  BUILD_opencv_rgbd=OFF
  BUILD_opencv_saliency=OFF
  BUILD_opencv_sfm=OFF
  BUILD_opencv_stereo=OFF
  BUILD_opencv_structured_light=OFF
  BUILD_opencv_surface_matching=OFF
  BUILD_opencv_text=OFF
  BUILD_opencv_xfeatures2d=OFF
  BUILD_opencv_xobjdetect=OFF
  BUILD_opencv_xphoto=OFF
)

hunter_config(OpenCV VERSION ${HUNTER_OpenCV_VERSION} CMAKE_ARGS ${opencv_cmake_args})

##############################################
# dlib
##############################################

set(dlib_cmake_args
  DLIB_HEADER_ONLY=OFF  #all previous builds were header on, so that is the default
  DLIB_ENABLE_ASSERTS=OFF #must be set on/off or debug/release build will differ and config will not match one
  DLIB_NO_GUI_SUPPORT=ON
  DLIB_ISO_CPP_ONLY=OFF # needed for directory navigation code (loading training data)
  DLIB_JPEG_SUPPORT=OFF  # https://github.com/hunter-packages/dlib/blob/eb79843227d0be45e1efa68ef9cc6cc187338c8e/dlib/CMakeLists.txt#L422-L432
  DLIB_LINK_WITH_SQLITE3=OFF
  DLIB_USE_BLAS=OFF
  DLIB_USE_LAPACK=OFF
  DLIB_USE_CUDA=OFF
  DLIB_PNG_SUPPORT=ON
  DLIB_GIF_SUPPORT=OFF
  DLIB_USE_MKL_FFT=OFF  
  HUNTER_INSTALL_LICENSE_FILES=dlib/LICENSE.txt
)

# Workarounds for partial c++11 in ndk10e + gcc toolchains:
# TODO: This could be managed by try_compile tests, but won't be needed after ndk17
if(ANDROID)
  # * https://github.com/ruslo/hunter/commit/08d25c51e8fa3f3fcfa73f655fe3a7d85d1b4109
  set(dlib_version VERSION 19.2-p1)
else()
  set(dlib_version ${HUNTER_dlib_VERSION})
endif()

hunter_config(dlib VERSION ${dlib_version} CMAKE_ARGS ${dlib_cmake_args})

##############################################
# nlohmann_json
##############################################

# Workarounds for partial c++11 in ndk10e + gcc toolchains:
# TODO: This could be managed by try_compile tests, but won't be needed after ndk17
if(ANDROID)
  # * error: 'struct lconv' has no member named 'decimal_point'
  set(nlohmann_json_version VERSION 2.1.1-p1)
else()
  set(nlohmann_json_version ${HUNTER_nlohmann_json_VERSION})  
endif()

hunter_config(nlohmann_json VERSION ${nlohmann_json_version})