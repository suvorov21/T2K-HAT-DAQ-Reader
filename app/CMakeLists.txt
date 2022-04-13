######################
#   Applications     #
######################

set(exe_sources Converter.cxx
                Monitor.cxx)
set(exe_libraries TCore)

pbuilder_executables(
        SOURCES ${exe_sources}
        TARGETS_VAR programs
        PROJECT_LIBRARIES ${exe_libraries}
        PUBLIC_EXTERNAL_LIBRARIES ${PUBLIC_EXT_LIBS}
        PRIVATE_EXTERNAL_LIBRARIES ${PRIVATE_EXT_LIBS}
)

# Installation of executable
pbuilder_component_install_and_export(
       COMPONENT Executables
       EXETARGETS ${programs}
)
