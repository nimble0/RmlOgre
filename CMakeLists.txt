cmake_minimum_required(VERSION 3.10)

project(RmlOgre VERSION 0.1 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(OGRE REQUIRED)
find_package(RmlUi REQUIRED)


add_library(RmlOgre)

target_compile_features(RmlOgre PUBLIC cxx_std_17)
target_compile_options(RmlOgre PRIVATE
	$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
		-Wall -Wextra -Wpedantic -Wno-unused-parameter>
	$<$<CXX_COMPILER_ID:MSVC>:
		/W4>
)

target_include_directories(RmlOgre
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>

	SYSTEM PRIVATE
		${OGRE_INCLUDE_DIR}
		${OGRE_INCLUDE_DIR}/Hlms/Common
)

target_sources(RmlOgre
	PRIVATE
		src/RmlOgre/RenderInterface.cpp
)

target_link_libraries(RmlOgre
	${OGRE_LIBRARIES}
	${OGRE_HlmsUnlit_LIBRARIES}

	RmlUi::RmlUi
)

add_subdirectory("example")
