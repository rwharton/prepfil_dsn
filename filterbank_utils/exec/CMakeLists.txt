add_executable(prepfil prepfil.cpp)
target_link_libraries(prepfil
  filterbank_utils_static
  ${EXTERNAL_LIBS}
)

add_executable(mkfb mkfb.cpp)
target_link_libraries(mkfb
  filterbank_utils_static
  ${EXTERNAL_LIBS}
)

add_executable(tst test.cpp)
target_link_libraries(tst
  filterbank_utils_static
  ${EXTERNAL_LIBS}
)

install(
  TARGETS prepfil mkfb
  RUNTIME
  DESTINATION ${INSTALL_BIN_DIR}
)
