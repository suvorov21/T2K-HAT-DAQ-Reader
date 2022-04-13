# CMakeLists for src
# Author: M. Guigue
# Date: Jul 2, 2020

######################
#   Core             #
######################

configure_file( T2KConstants.h.in ${CMAKE_CURRENT_SOURCE_DIR}/T2KConstants.h )

set (CORE_DICT_HEADERFILES
    DAQ.h
    datum_decoder.h
    fdecoder.h
    frame.h
    Mapping.h
    EventDisplay.hxx
    platform_spec.h
    Interface.hxx
    SetT2KStyle.hxx
)

set (CORE_SOURCEFILES
    DAQ.cxx
    datum_decoder.cxx
    frame.c
    Mapping.cxx
    EventDisplay.cxx
    platform_spec.h
    Interface.cxx
)

# If there were other libraries in this package on which this library depends, then they would be put in this variable
set(PACKAGE_LIBS TRawEvent_daq_reader)

set( CORE_LINKDEF_HEADERFILE LinkDef/CCoreLinkDef.h )
set( CORE_SOURCEFILES ${CORE_SOURCEFILES} ${CMAKE_CURRENT_BINARY_DIR}/CCoreDict.cxx )
set( CORE_DICT_PCMFILE ${CMAKE_CURRENT_BINARY_DIR}/CCoreDict_rdict.pcm )

##################################################
set( EVENT_SOURCEFILES ${EVENT_SOURCEFILES} ${CMAKE_CURRENT_BINARY_DIR}/CCoreDict.cxx )
ROOT_GENERATE_DICTIONARY( CCoreDict ${CORE_DICT_HEADERFILES} LINKDEF ${CORE_LINKDEF_HEADERFILE}  OPTIONS -inlineInputHeader )

configure_file( T2KConstants.h.in ${CMAKE_CURRENT_BINARY_DIR}/T2KConstants.h @ONLY )

pbuilder_install_files(${LIB_INSTALL_DIR} ${CORE_DICT_PCMFILE})

# ###########
# # Library #
# ###########
message("here")
message("${PUBLIC_EXT_LIBS}")
message("${PRIVATE_EXT_LIBS}")
message("${PACKAGE_LIBS}")

pbuilder_library(
        TARGET TCore
        SOURCES ${CORE_SOURCEFILES}
        PROJECT_LIBRARIES ${PACKAGE_LIBS}
        PUBLIC_EXTERNAL_LIBRARIES ${PUBLIC_EXT_LIBS}
        PRIVATE_EXTERNAL_LIBRARIES ${PRIVATE_EXT_LIBS}
)
pbuilder_component_install_and_export(
        COMPONENT Library
        LIBTARGETS TCore
)

pbuilder_install_headers(${CORE_DICT_HEADERFILES})

