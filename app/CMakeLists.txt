######################
#   Applications     #
######################

add_executable(Converter Converter.cxx)
target_link_libraries(Converter TCore ${ROOT_LIBRARIES})
