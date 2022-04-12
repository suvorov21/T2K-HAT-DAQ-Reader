# Minimum cmake verison 3.1 required for the variable CMAKE_CXX_STANDARD
cmake_minimum_required(VERSION 3.1)

# ##############################################################################
# setup #
# ##############################################################################

set(PROJECT_NAME DAQ_reader)
set(PROJECT_VERSION 1.0)

project(${PROJECT_NAME} VERSION ${PROJECT_VERSION})

set(CMAKE_CXX_STANDARD 14) # Enable c++14 standard

######
# ROOT
######

list (APPEND Root_COMPONENTS RIO Net Hist Graf Graf3d Gpad Gui Tree Rint Postscript Matrix Physics MathCore Thread)
find_package (ROOT 6.00 REQUIRED COMPONENTS ${Root_COMPONENTS})
if (ROOT_FOUND)

else (ROOT_FOUND)
    message(FATAL "Unable to find ROOT")
endif (ROOT_FOUND)
include(${ROOT_USE_FILE})
include_directories (${ROOT_INCLUDE_DIR})

#########
# sources
#########

include_directories (
    ${ROOT_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/app
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/external/hat_event
    ${PROJECT_SOURCE_DIR}/external/hat_event/src/
)

add_subdirectory (external/hat_event/)
add_subdirectory (src)
add_subdirectory (app)
