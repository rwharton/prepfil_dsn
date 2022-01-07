add_custom_command(
  OUTPUT sigproc_file_test_input_files
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/input .
)

add_executable(sigproc_file sigproc_file.cpp sigproc_file_test_input_files)

add_test(sigproc_file sigproc_file)

target_link_libraries(sigproc_file
  filterbank_utils_static
  ${EXTERNAL_LIBS}
)