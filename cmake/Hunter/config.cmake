### Configure XGBOOST w/ cereal ###
set(XGBOOST_CMAKE_ARGS
  XGBOOST_USE_HALF=ON
  XGBOOST_USE_CEREAL=ON
  )
if(DRISHTI_TEST_BUILD_MIN_SIZE)
  list(APPEND XGBOOST_CMAKE_ARGS XGBOOST_DO_LEAN=ON)
else()
  list(APPEND XGBOOST_CMAKE_ARGS XGBOOST_DO_LEAN=OFF)
endif()
hunter_config(xgboost VERSION ${HUNTER_xgboost_VERSION} CMAKE_ARGS ${XGBOOST_CMAKE_ARGS})

### Configure drishti as submodule ###

if(DRISHTI_AS_SUBMODULE)
  hunter_config(drishti GIT_SUBMODULE "src/3rdparty/drishti")
else()
  hunter_config(drishti VERSION ${HUNTER_drishti_VERSION})
endif()

# Workaround from:
# * https://github.com/ruslo/hunter/commit/08d25c51e8fa3f3fcfa73f655fe3a7d85d1b4109
if(ANDROID)
  hunter_config(dlib VERSION 19.2-p1)
endif()
