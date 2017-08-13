# set(XGBOOST_CMAKE_ARGS
#   XGBOOST_USE_HALF=ON
#   XGBOOST_USE_CEREAL=ON
#   )
# if(DRISHTI_BUILD_MIN_SIZE)
#   list(APPEND XGBOOST_CMAKE_ARGS XGBOOST_DO_LEAN=ON)
# else()
#   list(APPEND XGBOOST_CMAKE_ARGS XGBOOST_DO_LEAN=OFF)
# endif()

# hunter_config(xgboost GIT_SUBMODULE "src/3rdparty/xgboost" CMAKE_ARGS ${XGBOOST_CMAKE_ARGS})

# hunter_config(drishti GIT_SUBMODULE "src/3rdparty/drishti" CMAKE_ARGS ${XGBOOST_CMAKE_ARGS})

hunter_config(drishti GIT_SUBMODULE "src/3rdparty/drishti")
