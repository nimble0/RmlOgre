add_executable(RmlOgreExample WIN32 src/main.cpp)

target_compile_features(RmlOgreExample PUBLIC cxx_std_17)
target_compile_options(RmlOgreExample PRIVATE
	$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
		-Wall -Wextra -Wpedantic -Wno-unused-parameter>
	$<$<CXX_COMPILER_ID:MSVC>:
		/W4>
)

target_include_directories(RmlOgreExample
	PRIVATE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_include_directories(RmlOgreExample
	SYSTEM PRIVATE
		${OGRE_INCLUDE_DIR}
		${OGRE_INCLUDE_DIR}/Hlms/Common
		${OGRE_INCLUDE_DIR}/Hlms/Unlit
)

target_link_libraries(RmlOgreExample
	${OGRE_LIBRARIES}
	${OGRE_HlmsUnlit_LIBRARIES}
	${OGRE_RenderSystem_Direct3D11_LIBRARIES}
	${OGRE_RenderSystem_GL3Plus_LIBRARIES}
	${OGRE_RenderSystem_GLES2_LIBRARIES}
	${OGRE_RenderSystem_Metal_LIBRARIES}
	${OGRE_RenderSystem_Vulkan_LIBRARIES}

	RmlUi::RmlUi
	RmlOgre::RmlOgre
)

set(BIN_CFG_FILES
	plugins.cfg
	plugins_d.cfg
	resources.cfg
)

foreach(FILE ${BIN_CFG_FILES})
	configure_file("${PROJECT_SOURCE_DIR}/bin_cfg/${FILE}" "." COPYONLY)

	file(READ "${CMAKE_CURRENT_BINARY_DIR}/${FILE}" FILE_CONTENT)
	string(REPLACE "[OGRE_PLUGIN_DIR]" ${OGRE_PLUGIN_DIR_REL} FILE_CONTENT "${FILE_CONTENT}" )
	string(REPLACE "[OGRE_MEDIA_DIR]" ${OGRE_MEDIA_DIR} FILE_CONTENT "${FILE_CONTENT}" )
	file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${FILE}" "${FILE_CONTENT}")
endforeach()

file(CREATE_LINK "${PROJECT_SOURCE_DIR}/media" "${CMAKE_CURRENT_BINARY_DIR}/media" COPY_ON_ERROR SYMBOLIC)
