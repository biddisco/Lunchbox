
# Copyright (c) 2012 Stefan Eilemann <eile@eyescale.ch>

message ("I am here " ${OUTPUT_INCLUDE_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.in.h
  ${OUTPUT_INCLUDE_DIR}/lunchbox/version.h @ONLY)
