cmake_minimum_required(VERSION 2.8)
project(webfort)

set(DFHACK_VER 0.34.11)
set(DFHACK_REL r5)
add_definitions(-DDFHACK_VERSION="${DFHACK_VER}-${DFHACK_REL}")
# dfhack path
set(DH dfhack)

# A list of source files
set(SRC
    webfort.cpp
    server.cpp
)
# A list of headers
set(HDRS
    server.h
)
set_source_files_properties(${HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

# mash them together (headers are marked as headers and nothing will try to compile them)
list(APPEND SRC ${HDRS})

# option to use a thread for no particular reason
# OPTION(SKELETON_THREAD "Use threads in the skeleton plugin." ON)

#linux
if(UNIX)
    add_definitions(
        -std=c++11
        -DLINUX_BUILD
        -D_WEBSOCKETPP_CPP11_STL_
    )
    set(WS_BOOST_LIBS system)
    set(PROJECT_LIBS
        pthread
        ${PROJECT_LIBS}
    )
    set(DF_ROOT ${CMAKE_SOURCE_DIR}/df_linux)
# windows
else(UNIX)
    if(NOT MSVC10)
        message(FATAL_ERROR "Visual Studio 2010 required.")
    endif()
    add_definitions(
        -D_WEBSOCKETPP_CPP11_FUNCTIONAL_
        -D_WEBSOCKETPP_CPP11_SYSTEM_ERROR_
        -D_WEBSOCKETPP_CPP11_RANDOM_DEVICE_
        -D_WEBSOCKETPP_CPP11_MEMORY_
        )
    set(PROJECT_LIBS
        ${PROJECT_LIBS}
        $(NOINHERIT)
    )
endif(UNIX)

add_library(webfort.plug SHARED ${SRC})

# DFHack + friends
include_directories(
    ${DH}/library/include
    ${DH}/library/proto
    ${DH}/depends/protobuf
    ${DH}/depends/tthread
)

find_library(DFHACK_LIBRARY dfhack ${DH}/build/library)
find_library(TINYTHREAD_LIBRARY dfhack-tinythread ${DH}/build/depends/tthread)
target_link_libraries(webfort.plug
    ${DFHACK_LIBRARY}
    ${TINYTHREAD_LIBRARY}
)

# SDL Headers
find_package(SDL 1.2 REQUIRED)
include_directories(${SDL_INCLUDE_DIRS})

# Boost/Asio
set(Boost_FIND_REQUIRED TRUE)
find_package(Boost 1.39.0 COMPONENTS "${WS_BOOST_LIBS}")

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(webfort.plug ${Boost_LIBRARIES})
endif()

# websocketpp
include_directories(websocketpp)

# Installing
install(TARGETS webfort.plug
    RUNTIME DESTINATION ${DF_ROOT}/hack/plugins
    LIBRARY DESTINATION ${DF_ROOT}/hack/plugins
)
