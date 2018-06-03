option(DRISHTI_SDK_TEST_USE_LOCAL_DRISHTI "Use drishti via add_subdirectory()" OFF)

if(DRISHTI_SDK_TEST_USE_LOCAL_DRISHTI)
  option(DRISHTI_BUILD_EXAMPLES "Build example" OFF)
  option(DRISHTI_BUILD_OGLES_GPGPU "Build with OGLES_GPGPU" ON)
  add_subdirectory(3rdparty/drishti)
endif()

add_subdirectory(app)
