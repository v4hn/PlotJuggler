function(find_or_download_dependencies)

    set(CPM_USE_LOCAL_PACKAGES ON)

    ##### fastCDR ######
    find_package(fastcdr QUIET)

    if (fastcdr_FOUND)
        message(STATUS "Found fastcdr")

    elseif(NOT TARGET fastcdr)
        message(STATUS "fastcdr not found, downloading")
        CPMAddPackage(
            NAME fastcdr_imported
            GITHUB_REPOSITORY eProsima/Fast-CDR
            GIT_TAG v2.2.6
            OPTIONS "BUILD_SHARED_LIBS OFF"
        )
    endif()

    ##### backward_cpp ######
    find_package(backward_cpp QUIET)

    if (backward_cpp_FOUND)
        message(STATUS "Found backward_cpp")
    elseif(NOT TARGET backward_cpp)
        CPMAddPackage("gh:bombela/backward-cpp@1.6")
    endif()

    ##### nlohmann_json ######

    CPMAddPackage("gh:nlohmann/json@3.11.3")

    ##### Lua + Sol2 ######

    # find_package(Lua QUIET)

    if(LUA_FOUND)
        message(STATUS "Found lua")
    elseif (NOT TARGET lua)
        CPMAddPackage(
        NAME lua
        GIT_REPOSITORY https://github.com/lua/lua.git
        VERSION 5.4.7
        DOWNLOAD_ONLY YES
        )
        # lua has no CMake support, so we create our own target
        if (lua_ADDED)
            file(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
            list(REMOVE_ITEM lua_sources "${lua_SOURCE_DIR}/lua.c" "${lua_SOURCE_DIR}/luac.c" "${lua_SOURCE_DIR}/onelua.c")
            add_library(lua_static STATIC ${lua_sources})
            target_include_directories(lua_static SYSTEM PUBLIC $<BUILD_INTERFACE:${lua_SOURCE_DIR}>)
            set_property(TARGET lua_static PROPERTY POSITION_INDEPENDENT_CODE ON)

            install(
                TARGETS lua_static
                EXPORT ${PROJECT_NAME}Targets
                LIBRARY DESTINATION lib
                ARCHIVE DESTINATION lib
                RUNTIME DESTINATION bin
                INCLUDES DESTINATION include
                )

            set(LUA_INCLUDE_DIR "${lua_SOURCE_DIR}" CACHE PATH "Lua include directory" FORCE)
            set(LUA_LIBRARY lua_static CACHE STRING "Lua library" FORCE)

        endif()
    endif()

    add_library(Lua::Lua INTERFACE IMPORTED)
    set_target_properties(Lua::Lua PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${LUA_LIBRARY}"
    )

    ########################################

    CPMAddPackage("gh:ThePhD/sol2@3.3.0")

    ########################################

    CPMAddPackage(
        NAME data_tamer
        GITHUB_REPOSITORY PickNikRobotics/data_tamer
        GIT_TAG 1.0.2
        DOWNLOAD_ONLY YES
    )

    if (data_tamer_ADDED)
        # we are interested only in data_tamer_parser includes
        add_library(data_tamer_parser INTERFACE)
        target_include_directories(data_tamer_parser INTERFACE "${data_tamer_SOURCE_DIR}/data_tamer_cpp/include")
    endif()

    ########################################

    find_package(ZSTD QUIET)

    if (ZSTD_FOUND)
        message(STATUS "Found ZSTD")
    elseif(NOT TARGET zstd)
        ### zstd ###
        CPMAddPackage(
            NAME zstd
            GITHUB_REPOSITORY facebook/zstd
            GIT_TAG v1.5.6
            DOWNLOAD_ONLY YES
        )
        set(ZSTD_BUILD_STATIC ON  CACHE BOOL " " FORCE)
        set(ZSTD_BUILD_SHARED OFF CACHE BOOL " " FORCE)
        set(ZSTD_LEGACY_SUPPORT OFF   CACHE BOOL " " FORCE)
        set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL " " FORCE)
        set(ZSTD_BUILD_TESTS OFF   CACHE BOOL " " FORCE)
        set(ZSTD_BUILD_CONTRIB OFF  CACHE BOOL " " FORCE)
        set(ZSTD_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
        set(ZSTD_MULTITHREAD_SUPPORT OFF CACHE BOOL " " FORCE)
        set(ZSTD_LEGACY_SUPPORT OFF CACHE BOOL " " FORCE)
        set(ZSTD_ZLIB_SUPPORT OFF CACHE BOOL " " FORCE)
        set(ZSTD_LZ4_SUPPORT OFF CACHE BOOL " " FORCE)
        set(ZSTD_LZMA_SUPPORT OFF CACHE BOOL " " FORCE)
        set(ZSTD_ZDICT_SUPPORT OFF CACHE BOOL " " FORCE)
        set(ZSTD_PROGRAMS "" CACHE STRING " " FORCE)

        add_subdirectory(${zstd_SOURCE_DIR}/build/cmake ${zstd_BINARY_DIR})
        set(ZSTD_INCLUDE_DIR ${zstd_SOURCE_DIR}/lib)
        set(ZSTD_LIBRARY libzstd_static)
    endif()

    add_library(zstd::libzstd_static INTERFACE IMPORTED)
    set_target_properties(zstd::libzstd_static PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${ZSTD_LIBRARY}"
    )

    ########################################
    find_package(LZ4 QUIET)

    if (LZ4_FOUND)
        message(STATUS "Found LZ4")
    elseif(NOT TARGET lz4_static)
        ### lz4 ###
        CPMAddPackage(
            NAME lz4
            GITHUB_REPOSITORY lz4/lz4
            GIT_TAG v1.10.0
            DOWNLOAD_ONLY YES
        )
        file(GLOB LZ4_SOURCES ${lz4_SOURCE_DIR}/lib/*.c)
        add_library(lz4_static STATIC ${LZ4_SOURCES})
        target_include_directories(lz4_static PUBLIC ${lz4_SOURCE_DIR}/lib)

        set(LZ4_INCLUDE_DIR ${lz4_SOURCE_DIR}/lib)
        SET(LZ4_LIBRARIES lz4_static)
        mark_as_advanced(LZ4_INCLUDE_DIR LZ4_LIBRARIES)
    endif()

    add_library(lz4::lz4_static INTERFACE IMPORTED)
    set_target_properties(lz4::lz4_static PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LZ4_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${LZ4_LIBRARIES}"
    )

endfunction()
