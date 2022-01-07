protobuf_generate_cpp(PROTO_SRC PROTO_HDR MakeFilterbankConfig.proto)
file(RELATIVE_PATH PROTO_SRC_REL ${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_SRC})
#file(RELATIVE_PATH PROTO_HDR_REL ${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_HDR})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SRCS
  SigProc.cpp
  SigProcHeader.cpp
  SigProcUtil.cpp
  RFIMask.cpp
  MakeFilterbankConfig.cpp
  MakeFilterbank.cpp
  ScanFile.cpp
  PulsarCatalog.cpp
  Barycenter.cpp
  utils.cpp
  ${PROTO_SRC_REL}
)

# there is a big variable in that class, so turn off var tracking
set_source_files_properties(PulsarCatalog.cpp COMPILE_FLAGS -fno-var-tracking)

if (${FFTW_FOUND})
  set(SRCS "${SRCS};BaselineRemover_CPU.cpp")
else()
  set(SRCS "${SRCS};BaselineRemover_NO_CPU.cpp")
endif()

if (NOT ${CUDA_FOUND})
  set(SRCS "${SRCS};BaselineRemover_NO_GPU.cpp")
endif()

add_library(lib OBJECT ${SRCS})
set(LIBS "${LIBS};$<TARGET_OBJECTS:lib>" PARENT_SCOPE)
