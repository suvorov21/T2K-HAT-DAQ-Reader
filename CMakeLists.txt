# Minimum cmake verison 3.1 required for the variable CMAKE_CXX_STANDARD
cmake_minimum_required(VERSION 3.1)

# ##############################################################################
# setup #
# ##############################################################################

set(PROJECT_NAME DAQ_reader)
set(PROJECT_VERSION 1.0)

cmake_policy(SET CMP0048 NEW) # version in project()
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION})

set(CMAKE_CXX_STANDARD 14) # Enable c++14 standard

list(APPEND CMAKE_MODULE_PATH
            ${PROJECT_SOURCE_DIR}/cmake)
include( PackageBuilder )

pbuilder_prepare_project()

######
# ROOT
######

list (APPEND Root_COMPONENTS RIO Net Hist Graf Graf3d Gpad Gui Tree Rint Postscript Matrix Physics MathCore Thread)
find_package (ROOT 6.00 REQUIRED COMPONENTS ${Root_COMPONENTS})
if (ROOT_FOUND)
    pbuilder_add_ext_libraries (${ROOT_LIBRARIES})
else (ROOT_FOUND)
    message(FATAL "Unable to find ROOT")
endif (ROOT_FOUND)
include_directories (${ROOT_INCLUDE_DIR})

#########
# sources
#########

include_directories (BEFORE
    ${PROJECT_SOURCE_DIR}/app
    ${PROJECT_SOURCE_DIR}/src
)

add_subdirectory (src)
add_subdirectory (app)
