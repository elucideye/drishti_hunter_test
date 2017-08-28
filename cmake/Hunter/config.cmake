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
hunter_config(drishti GIT_SUBMODULE "src/3rdparty/drishti")
