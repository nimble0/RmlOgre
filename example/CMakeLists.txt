include(${PROJECT_SOURCE_DIR}/cmake/OGRE.cmake)

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

target_compile_definitions(RmlOgreExample PRIVATE RMLOGRE_MEDIA_DIR=${RMLOGRE_MEDIA_DIR})

if(CMAKE_CONFIGURATION_TYPES)
	foreach(BUILD_TYPE ${CMAKE_CONFIGURATION_TYPES})
		message( STATUS "Copying DLLs and generating plugins.cfg for ${BUILD_TYPE}" )
		setupPluginFileFromTemplate(
			${OGRE-Next_DIR}
			${PROJECT_SOURCE_DIR}/cmake/templates/plugins.cfg.in
			${CMAKE_CURRENT_BINARY_DIR}/${BUILD_TYPE}
			${BUILD_TYPE}
			false
			false )

		configure_file(
			${PROJECT_SOURCE_DIR}/cmake/templates/resources.cfg.in
			${CMAKE_CURRENT_BINARY_DIR}/${BUILD_TYPE}/resources.cfg )
	endforeach()
else()
	message( STATUS "Copying DLLs and generating plugins.cfg for ${CMAKE_BUILD_TYPE}" )
	setupPluginFileFromTemplate(
		${OGRE-Next_DIR}
		${PROJECT_SOURCE_DIR}/cmake/templates/plugins.cfg.in
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_BUILD_TYPE}
		false
		false )

	configure_file(
		${PROJECT_SOURCE_DIR}/cmake/templates/resources.cfg.in
		${CMAKE_CURRENT_BINARY_DIR}/resources.cfg )
endif()
