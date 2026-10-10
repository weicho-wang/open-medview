cmake_minimum_required(VERSION 3.21)

project(OpenMedView VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(GDCM CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

if(DEFINED GDCM_USE_FILE AND GDCM_USE_FILE)
    include(${GDCM_USE_FILE})
endif()

add_executable(OpenMedView
    src/main.cpp
    src/Anonymizer.cpp
    src/DicomReader.cpp
    src/FileScanner.cpp
    src/RuleEngine.cpp
)

target_include_directories(OpenMedView
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(OpenMedView
    PRIVATE
        gdcmMSFF
        gdcmIOD
        gdcmDSED
        gdcmDICT
        gdcmCommon
        nlohmann_json::nlohmann_json
)

if(COMMAND gdcm_configure_target)
    gdcm_configure_target(OpenMedView)
endif()
