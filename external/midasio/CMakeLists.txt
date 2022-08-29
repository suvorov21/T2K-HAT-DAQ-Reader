# Minimum cmake verison 3.1 required for the variable CMAKE_CXX_STANDARD
cmake_minimum_required(VERSION 3.1)

# ##############################################################################
# setup #
# ##############################################################################

set(PROJECT_NAME midasio)
set(PROJECT_VERSION 1.0)

project(${PROJECT_NAME} VERSION ${PROJECT_VERSION})

# From https://blog.kitware.com/cmake-and-the-default-build-type/
# Set a default build type to Release if none was specified
# This has to be before Scarab PackageBuilder since the package builder defines the default mode to DEBUG...
set(default_build_type "Release")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

list(APPEND CMAKE_MODULE_PATH
        ${PROJECT_SOURCE_DIR}/../hat_event/cmake)
include(PackageBuilder)

pbuilder_prepare_project()


set(MIDASIO_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# MIDASIO
file(GLOB MIDASIO_SOURCE_FILES
        "${MIDASIO_DIR}/*.cxx"
        )

file(GLOB MIDASIO_HEADER_FILES
        "${MIDASIO_DIR}/*.h"
        )

file(GLOB MIDASIO_EXE_FILES
        "${MIDASIO_DIR}/test_midasio.cxx"
        )

# Excluding exectuables
list( REMOVE_ITEM MIDASIO_SOURCE_FILES ${MIDASIO_DIR}/test_midasio.cxx )

# force using C++ compiler
foreach(SOURCE_FILE_PATH ${MIDASIO_SOURCE_FILES})
    SET_SOURCE_FILES_PROPERTIES( ${MIDASIO_SOURCE_FILES} PROPERTIES LANGUAGE CXX )
ENDFOREACH()

set(MIDASIO_INCLUDE_DIR
        ${MIDASIO_SOURCE_FILES}
        )

include_directories( ${MIDASIO_INCLUDE_DIR} )

set ( MIDASIO_DEP_LIBRARIES )

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -lz -Wno-unused-command-line-argument")

pbuilder_library(
        TARGET midasio
        SOURCES ${MIDASIO_SOURCE_FILES}
        PROJECT_LIBRARIES ${MIDASIO_DEP_LIBRARIES}
        PUBLIC_EXTERNAL_LIBRARIES ${PUBLIC_EXT_LIBS}
        PRIVATE_EXTERNAL_LIBRARIES ${PRIVATE_EXT_LIBS}
)

pbuilder_install_headers (${MIDASIO_HEADER_FILES})

pbuilder_executables(
        SOURCES ${MIDASIO_EXE_FILES}
        PROJECT_LIBRARIES midasio
        PUBLIC_EXTERNAL_LIBRARIES ${PUBLIC_EXT_LIBS}
        PRIVATE_EXTERNAL_LIBRARIES ${PRIVATE_EXT_LIBS}
)

pbuilder_component_install_and_export(
        COMPONENT Library Executable
        LIBTARGETS midasio
        EXETARGETS test_midasio
)

configure_file(midasioConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/midasioConfig.cmake @ONLY)
pbuilder_do_package_config()

