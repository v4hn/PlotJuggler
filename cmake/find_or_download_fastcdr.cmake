function(find_or_download_fastcdr)

  # fastCDR ######
  find_package(fastcdr QUIET)

  if(fastcdr_FOUND)
    message(STATUS "Found fastcdr in system")
    add_library(fastcdr::fastcdr ALIAS fastcdr)

  elseif(NOT TARGET fastcdr)
    message(STATUS "fastcdr not found, downloading")
    cpmaddpackage(
      NAME
      fastcdr
      URL
      https://github.com/eProsima/Fast-CDR/archive/refs/tags/v2.3.0.zip
      OPTIONS
      "BUILD_SHARED_LIBS OFF"
      "BUILD_TESTING OFF")
    set_target_properties(fastcdr PROPERTIES POSITION_INDEPENDENT_CODE ON)
    add_library(fastcdr::fastcdr ALIAS fastcdr)
  endif()

endfunction()
