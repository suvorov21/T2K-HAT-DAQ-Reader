# CMakeLists for src
# Author: M. Guigue
# Date: Jul 2, 2020

######################
#   Core             #
######################

configure_file( T2KConstants.h.in ${CMAKE_CURRENT_SOURCE_DIR}/T2KConstants.h )
pbuilder_install_headers( ${CMAKE_CURRENT_SOURCE_DIR}/T2KConstants.h )


set (CORE_DICT_HEADERFILES
    DAQ.h
    datum_decoder.h
    fdecoder.h
    frame.h
    Mapping.h
    EventDisplay.hxx
    platform_spec.h
    Interface.hxx
)

set( CORE_HEADERFILES
    ${CORE_DICT_HEADERFILES}
)

set (CORE_DEP_LIBRARIES
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


set( CORE_LINKDEF_HEADERFILE LinkDef/CCoreLinkDef.h )
set( CORE_SOURCEFILES ${CORE_SOURCEFILES} ${CMAKE_CURRENT_BINARY_DIR}/CCoreDict.cxx )
set( CORE_DICT_PCMFILE ${CMAKE_CURRENT_BINARY_DIR}/CCoreDict_rdict.pcm )

##################################################

ROOT_GENERATE_DICTIONARY( CCoreDict ${CORE_DICT_HEADERFILES} LINKDEF ${CORE_LINKDEF_HEADERFILE}  OPTIONS -inlineInputHeader )
pbuilder_install_files( ${LIB_INSTALL_DIR} ${CORE_DICT_PCMFILE} )

# ###########
# # Library #
# ###########

pbuilder_library( CCore CORE_SOURCEFILES CORE_DEP_LIBRARIES )
pbuilder_install_headers (${CORE_HEADERFILES})

# configure_file( T2KConstants.h.in ${CMAKE_CURRENT_BINARY_DIR}/T2KConstants.h )
configure_file( T2KConstants.h.in ${CMAKE_CURRENT_BINARY_DIR}/T2KConstants.h @ONLY )
pbuilder_install_headers( ${CMAKE_CURRENT_BINARY_DIR}/T2KConstants.h )

