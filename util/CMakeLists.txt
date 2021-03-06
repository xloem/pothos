########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8.9)
project(PothosUtil CXX)

find_package(Pothos CONFIG REQUIRED)

########################################################################
## Feature registration
########################################################################
include(FeatureSummary)
include(CMakeDependentOption)
cmake_dependent_option(ENABLE_UTILITY "Enable Pothos Util component" ON "ENABLE_LIBRARY" OFF)
add_feature_info(Utility ENABLE_UTILITY "Common utility modules shared by components")
if (NOT ENABLE_UTILITY)
    return()
endif()

########################################################################
# Compiler support
########################################################################
add_subdirectory(Compiler)

########################################################################
# Eval support
########################################################################
get_filename_component(MUPARSERX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/muparserx/parser" ABSOLUTE)
include_directories(${MUPARSERX_ROOT})
file(GLOB MUPARSERX_SOURCES "${MUPARSERX_ROOT}/*.cpp")

POTHOS_MODULE_UTIL(
    TARGET EvalSupport
    SOURCES
        EvalEnvironment.cpp
        EvalEnvironmentListParsers.cpp
        TestEvalExpression.cpp
        BlockEval.cpp
        ${MUPARSERX_SOURCES}
    DESTINATION util
)

########################################################################
# JSON support (using POCO JSON)
########################################################################
POTHOS_MODULE_UTIL(
    TARGET JSONSupport
    SOURCES JSONSerialization.cpp TestJSONSerialization.cpp
    DESTINATION util
)

########################################################################
# Misc support (used by GUI)
# This module used heavily by the GUI but is not depedent on the GUI
# so it can be deployed on remote machines without the GUI installed.
########################################################################
set(SOURCES
    ConvertContainers.cpp
    DeviceInfoUtils.cpp
    DocUtils.cpp
    TestDocUtils.cpp
)

POTHOS_MODULE_UTIL(
    TARGET MiscSupport
    SOURCES ${SOURCES}
    DESTINATION util
)

########################################################################
# CPU device info for windows
########################################################################
if (WIN32)
POTHOS_MODULE_UTIL(
    TARGET WindowsGetLogicalProcessorInfo
    SOURCES WindowsGetLogicalProcessorInfo.cpp
    DESTINATION util
)
endif()
