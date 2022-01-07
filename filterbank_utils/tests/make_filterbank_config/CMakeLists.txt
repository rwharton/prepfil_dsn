add_custom_command(
  OUTPUT make_filterbank_config_test_input_files
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/conf .
)

add_executable(make_filterbank_config make_filterbank_config.cpp make_filterbank_config_test_input_files)

add_test(make_filterbank_config make_filterbank_config)

target_link_libraries(make_filterbank_config
  filterbank_utils_static
  ${EXTERNAL_LIBS}
)