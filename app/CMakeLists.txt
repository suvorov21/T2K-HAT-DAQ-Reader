######################
#   Applications     #
######################

set (APPLICATIONS_HEADERFILES

)

set (APPLICATIONS_DEP_LIBRARIES
    CCore
)


set (APPLICATIONS_PROGRAMS
    Converter
    Monitor
)

pbuilder_executables( APPLICATIONS_PROGRAMS APPLICATIONS_DEP_LIBRARIES )
pbuilder_install_headers (${APPLICATIONS_HEADERFILES})