get_directory_property(isPlugin PARENT_DIRECTORY)

project(webfort)

if (NOT isPlugin)
	cmake_minimum_required(VERSION 2.8)
	set(DFHACK_VER 0.34.11)
	set(DFHACK_REL r5)
	add_definitions(-DDFHACK_VERSION="${DFHACK_VER}-${DFHACK_REL}")
	# dfhack path
	set(DH dfhack)
endif()

# A list of source files
set(SRC
	webfort.cpp
	server.cpp
)
# A list of headers
set(HDRS
	shared.h
)
set_source_files_properties(${HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

# mash them together (headers are marked as headers and nothing will try to compile them)
list(APPEND SRC ${HDRS})

set(PROJECT_LIBS)

#linux
if(UNIX)
	add_definitions(
		-std=c++11
		-DLINUX_BUILD
		-D_WEBSOCKETPP_CPP11_STL_
	)
	list(APPEND PROJECT_LIBS pthread)
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
		-DNOMINMAX
		)
endif(UNIX)

# SDL Headers
find_package(SDL 1.2 REQUIRED)
include_directories(${SDL_INCLUDE_DIRS})

# Boost/Asio
set(Boost_FIND_REQUIRED TRUE)
set(WS_BOOST_LIBS system thread regex)
find_package(Boost 1.39.0 COMPONENTS "${WS_BOOST_LIBS}")

if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
	list(APPEND PROJECT_LIBS ${Boost_LIBRARIES})
endif()

# websocketpp
include_directories(websocketpp)

if (isPlugin)
	DFHACK_PLUGIN(webfort ${SRC} LINK_LIBRARIES dfhack-tinythread ${PROJECT_LIBS})
else()
	if (NOT UNIX)
		message(FATAL_ERROR "Building webfort out-of-tree not supported on windows.")
	endif()
	find_library(DFHACK_LIBRARY dfhack ${DH}/build/library)
	find_library(TINYTHREAD_LIBRARY dfhack-tinythread ${DH}/build/depends/tthread)
	list(APPEND PROJECT_LIBS
		${DFHACK_LIBRARY}
		${TINYTHREAD_LIBRARY}
	)

	include_directories(
		${DH}/library/include
		${DH}/library/proto
		${DH}/depends/protobuf
		${DH}/depends/tthread
	)

	add_library(webfort.plug SHARED ${SRC})
	target_link_libraries(webfort.plug ${PROJECT_LIBS})
	# Installing
	install(TARGETS webfort.plug
		RUNTIME DESTINATION ${DF_ROOT}/hack/plugins
		LIBRARY DESTINATION ${DF_ROOT}/hack/plugins
	)
endif()
