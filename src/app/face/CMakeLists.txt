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
  target_compile_definitions(drishti-face-test PUBLIC DRISHTI_TEST_BUILD_TESTS=1)
endif()
if(DRISHTI_HUNTER_TEST_ADD_TO_STRING)
  target_compile_definitions(drishti-face-test PUBLIC DRISHTI_HUNTER_TEST_ADD_TO_STRING=1)
endif()
install(TARGETS drishti-face-test DESTINATION bin)