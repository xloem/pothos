########################################################################
## Feature registration
########################################################################
cmake_dependent_option(ENABLE_COMMS_MATH "Enable Pothos Comms.Math component" ON "ENABLE_COMMS" OFF)
add_feature_info("  Math" ENABLE_COMMS_MATH "Mathematical operators for sample streams")
if (NOT ENABLE_COMMS_MATH)
    return()
endif()

########################################################################
# Math blocks module
########################################################################
if(MSVC)
    add_definitions(/bigobj) #arithmetic too large in debug mode
endif(MSVC)

POTHOS_MODULE_UTIL(
    TARGET MathBlocks
    SOURCES
        Arithmetic.cpp
        TestArithmeticBlocks.cpp
    DESTINATION comms
    ENABLE_DOCS
)
