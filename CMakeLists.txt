cmake_minimum_required(VERSION 3.10)

project(RmlOgre VERSION 0.1 LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(OGRE-Next REQUIRED)
find_package(RmlUi REQUIRED)


add_library(RmlOgre)
add_library(RmlOgre::RmlOgre ALIAS RmlOgre)

set(RMLOGRE_MEDIA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/media)

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
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_include_directories(RmlOgre
	SYSTEM PRIVATE
		${OGRE_INCLUDE_DIR}
		${OGRE_INCLUDE_DIR}/Hlms/Common
)

target_sources(RmlOgre
	PRIVATE
		src/RmlOgre/CompositorPassGeometry.cpp
		src/RmlOgre/RenderInterface.cpp
		src/RmlOgre/RenderObject.cpp
		src/RmlOgre/geometry.cpp
)

target_link_libraries(RmlOgre
	${OGRE_LIBRARIES}
	${OGRE_HlmsUnlit_LIBRARIES}

	RmlUi::RmlUi
)

set(PUBLIC_HEADERS
	src/RmlOgre/CompositorPassGeometry.hpp
	src/RmlOgre/CompositorPassGeometryDef.hpp
	src/RmlOgre/RenderInterface.hpp
	src/RmlOgre/RenderObject.hpp
)
set_target_properties(RmlOgre
	PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}"
)

configure_package_config_file(
	cmake/RmlOgreConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/RmlOgreConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RmlOgre
)
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/RmlOgreConfigVersion.cmake
	COMPATIBILITY SameMajorVersion
)

install(
	TARGETS RmlOgre
	EXPORT RmlOgreTargets
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/RmlOgre
)
install(
	EXPORT RmlOgreTargets
	FILE RmlOgreTargets.cmake
	NAMESPACE RmlOgre::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RmlOgre
)
install(
	FILES
		${CMAKE_CURRENT_BINARY_DIR}/RmlOgreConfig.cmake
		${CMAKE_CURRENT_BINARY_DIR}/RmlOgreConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RmlOgre
)

add_subdirectory("example")
