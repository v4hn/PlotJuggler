function(find_or_download_fastcdr)

    ##### fastCDR ######
    find_package(fastcdr QUIET)

    if (fastcdr_FOUND)
        message(STATUS "Found fastcdr in system")

    elseif(NOT TARGET fastcdr)
        message(STATUS "fastcdr not found, downloading")
        CPMAddPackage(
            NAME fastcdr
            URL https://github.com/eProsima/Fast-CDR/archive/refs/tags/v2.3.0.zip
            OPTIONS "BUILD_SHARED_LIBS OFF" "BUILD_TESTING OFF"
        )
        set_target_properties(fastcdr PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()

endfunction()
