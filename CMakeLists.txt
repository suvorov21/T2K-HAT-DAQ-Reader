# Minimum cmake verison 3.1 required for the variable CMAKE_CXX_STANDARD
cmake_minimum_required(VERSION 3.1)

# ##############################################################################
# setup #
# ##############################################################################

set(PROJECT_NAME daq_reader)
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

set(PBUILDER ON)
list(APPEND CMAKE_MODULE_PATH
        ${PROJECT_SOURCE_DIR}/cmake
        ${PROJECT_SOURCE_DIR}/external/hat_event/cmake)
include(PackageBuilder)

pbuilder_prepare_project()

set(CMAKE_CXX_STANDARD 14) # Enable c++14 standard


option( ENABLE_MIDASIO "Link the midasio libs to the project." ON )

######
# ROOT
######

list (APPEND Root_COMPONENTS RIO Net Hist Graf Graf3d Gpad Gui Tree Rint Postscript Matrix Physics MathCore Thread)
find_package (ROOT 6.00 REQUIRED COMPONENTS ${Root_COMPONENTS})
if (ROOT_FOUND)

else (ROOT_FOUND)
    message(FATAL "Unable to find ROOT")
endif (ROOT_FOUND)

if(ROOT_CXX_FLAGS MATCHES "-std=c\\+\\+17")
    set(CMAKE_CXX_STANDARD 17)
endif()

# include(${ROOT_USE_FILE})
include_directories (${ROOT_INCLUDE_DIR})
LIST(APPEND PUBLIC_EXT_LIBS ${ROOT_LIBRARIES})

pbuilder_add_submodule(hat_event ${PROJECT_SOURCE_DIR}/external/hat_event)

pbuilder_add_submodule(midasio ${PROJECT_SOURCE_DIR}/external/midasio)

#########
# sources
#########

include_directories (
    ${ROOT_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/app
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/external/hat_event
    ${PROJECT_SOURCE_DIR}/external/hat_event/src/
    ${PROJECT_SOURCE_DIR}/external/midasio
    ${PROJECT_SOURCE_DIR}/external/cli_parser/include
)

add_subdirectory (src)
add_subdirectory (app)

configure_file(${PROJECT_SOURCE_DIR}/daq_readerConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/daq_readerConfig.cmake @ONLY)
pbuilder_do_package_config()

configure_file(this_daq.sh.in this_daq.sh)
pbuilder_install_files(${BIN_INSTALL_DIR} ${CMAKE_CURRENT_BINARY_DIR}/this_daq.sh)

add_custom_target(install_${PROJECT_NAME}
        "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target install
        COMMENT "Installing ${PROJECT_NAME}")
