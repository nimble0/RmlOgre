#-------------------------------------------------------------------
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Finds the string "#define RESULT_NAME" in OgreBuildSettings.h
# (provided as a string in an argument)
function( findOgreBuildSetting OGRE_BUILD_SETTINGS_STR RESULT_NAME )
	string( FIND "${OGRE_BUILD_SETTINGS_STR}" "#define ${RESULT_NAME}" TMP_RESULT )
	if( NOT TMP_RESULT EQUAL -1 )
		set( ${RESULT_NAME} 1 PARENT_SCOPE )
	else()
		unset( ${RESULT_NAME} PARENT_SCOPE )
	endif()
endfunction()

#----------------------------------------------------------------------------------------

# On Windows it's a regular copy.
# On Apple it does nothing
# On Linux, it copies both libOgreMain.so.2.1 and its symbolic link libOgreMain.so
function( copyWithSymLink SRC DST )
	if( NOT APPLE )
		if( UNIX )
			get_filename_component( RESOLVED_LIB_PATH ${SRC} REALPATH )
			file( COPY ${RESOLVED_LIB_PATH} DESTINATION ${DST} )
		endif()
		file( COPY ${SRC} DESTINATION ${DST} )
	endif()
endfunction()

#----------------------------------------------------------------------------------------

# Finds if Ogre has been built a library with LIBRARY_NAME.dll,
# and if so sets the string for plugins.cfg template CFG_VARIABLE.
macro( findPluginAndSetPath SDK_DIR BIN_DIR BUILD_TYPE CFG_VARIABLE LIBRARY_NAME )
	set( REAL_LIB_PATH ${LIBRARY_NAME} )
	if( ${BUILD_TYPE} STREQUAL "Debug" )
		set( REAL_LIB_PATH ${REAL_LIB_PATH}_d )
	endif()

	if( WIN32 )
		set( REAL_LIB_PATH "${SDK_DIR}/bin/${BUILD_TYPE}/${REAL_LIB_PATH}.dll" )
	else()
		set( REAL_LIB_PATH "${SDK_DIR}/lib/${OGRE_NEXT_PREFIX}/${REAL_LIB_PATH}.so" )
	endif()

	if( EXISTS ${REAL_LIB_PATH} )
		# DLL Exists, set the variable for Plugins.cfg
		if( ${BUILD_TYPE} STREQUAL "Debug" )
			set( ${CFG_VARIABLE} "PluginOptional=${LIBRARY_NAME}_d" )
		else()
			set( ${CFG_VARIABLE} "PluginOptional=${LIBRARY_NAME}" )
		endif()

		# Copy the DLLs to the folders.
		copyWithSymLink( ${REAL_LIB_PATH} "${BIN_DIR}/Plugins" )
	endif()
endmacro()

#----------------------------------------------------------------------------------------

# Outputs TRUE into RESULT_VARIABLE if OGRE build uses next suffix
macro( isOgreNext SDK_DIR RESULT_VARIABLE )
	set( ${RESULT_VARIABLE} EXISTS "${OGRE_SDK}/include/OGRE-Next/OgreBuildSettings.h" )
endmacro()

#----------------------------------------------------------------------------------------

# Generates Plugins.cfg file out of user-editable plugins.cfg template.
# Will automatically disable those plugins that were not built.
# Copies all relevant DLLs: RenderSystem files, OgreOverlay, Hlms PBS & Unlit.
macro( setupPluginFileFromTemplate
	SDK_DIR
	TEMPLATE_FILE
	BIN_DIR
	BUILD_TYPE
	OGRE_USE_SCENE_FORMAT
	OGRE_USE_PLANAR_REFLECTIONS
)
	if( CMAKE_BUILD_TYPE )
		if( ${CMAKE_BUILD_TYPE} STREQUAL ${BUILD_TYPE} )
			set( OGRE_BUILD_TYPE_MATCHES 1 )
		endif()
	endif()

	# On non-Windows machines, we can only do Plugins for the current build.
	if( WIN32 OR OGRE_BUILD_TYPE_MATCHES )
		if( NOT APPLE )
			file( MAKE_DIRECTORY "${BIN_DIR}/Plugins" )
		endif()

		findPluginAndSetPath( ${SDK_DIR} ${BIN_DIR} ${BUILD_TYPE} OGRE_PLUGIN_RS_D3D11   RenderSystem_Direct3D11 )
		findPluginAndSetPath( ${SDK_DIR} ${BIN_DIR} ${BUILD_TYPE} OGRE_PLUGIN_RS_GL3PLUS RenderSystem_GL3Plus )
		findPluginAndSetPath( ${SDK_DIR} ${BIN_DIR} ${BUILD_TYPE} OGRE_PLUGIN_RS_VULKAN  RenderSystem_Vulkan )

		if( ${BUILD_TYPE} STREQUAL "Debug" )
			configure_file( ${TEMPLATE_FILE} ${BIN_DIR}/plugins_d.cfg )
		else()
			configure_file( ${TEMPLATE_FILE} ${BIN_DIR}/plugins.cfg )
		endif()

		# Copy
		# "${OGRE_SDK}/bin/${BUILD_TYPE}/OgreMain.dll" to "${CMAKE_CURRENT_BINARY_DIR}/${BUILD_TYPE}
		# and the other DLLs as well.

		# Lists of DLLs to copy
		set( OGRE_DLLS
			${OGRE_NEXT}Main
			${OGRE_NEXT}Overlay
			${OGRE_NEXT}HlmsPbs
			${OGRE_NEXT}HlmsUnlit
			)

		if( ${OGRE_USE_SCENE_FORMAT} )
			set( OGRE_DLLS ${OGRE_DLLS} ${OGRE_NEXT}SceneFormat )
		endif()

		if( NOT OGRE_BUILD_COMPONENT_ATMOSPHERE EQUAL -1 )
			set( OGRE_DLLS ${OGRE_DLLS} ${OGRE_NEXT}Atmosphere )
		endif()

		# Deal with OS and Ogre naming shenanigans:
		#	* OgreMain.dll vs libOgreMain.so
		#	* OgreMain_d.dll vs libOgreMain_d.so in Debug mode.
		if( WIN32 )
			set( DLL_OS_PREFIX "" )
			if( ${BUILD_TYPE} STREQUAL "Debug" )
				set( DLL_OS_SUFFIX "_d.dll" )
			else()
				set( DLL_OS_SUFFIX ".dll" )
			endif()
		else()
			set( DLL_OS_PREFIX "lib" )
			if( ${BUILD_TYPE} STREQUAL "Debug" )
				set( DLL_OS_SUFFIX "_d.so" )
			else()
				set( DLL_OS_SUFFIX ".so" )
			endif()
		endif()

		# On Windows DLLs are in build/bin/Debug & build/bin/Release;
		# On Linux DLLs are in build/Debug/lib.
		if( WIN32 )
			set( OGRE_DLL_PATH "${OGRE_SDK}/bin/${BUILD_TYPE}" )
		else()
			set( OGRE_DLL_PATH "${OGRE_SDK}/lib" )
		endif()

		# Do not copy anything if we don't find OgreMain.dll (likely Ogre was not build)
		list( GET OGRE_DLLS 0 DLL_NAME )
		if( EXISTS "${OGRE_DLL_PATH}/${DLL_OS_PREFIX}${DLL_NAME}${DLL_OS_SUFFIX}" )
			foreach( DLL_NAME ${OGRE_DLLS} )
				copyWithSymLink( "${OGRE_DLL_PATH}/${DLL_OS_PREFIX}${DLL_NAME}${DLL_OS_SUFFIX}"
				                 "${BIN_DIR}" )
			endforeach()
		endif()

		unset( OGRE_PLUGIN_RS_D3D11 )
		unset( OGRE_PLUGIN_RS_GL3PLUS )
		unset( OGRE_PLUGIN_RS_VULKAN )
	endif()

	unset( OGRE_BUILD_TYPE_MATCHES )
endmacro()

#----------------------------------------------------------------------------------------

# Creates Resources.cfg out of user-editable CMake/Templates/Resources.cfg.in
function( setupResourceFileFromTemplate TEMPLATE_FILE OUTPUT_FILE )
	message( STATUS "Generating ${OUTPUT_FILE} from template ${TEMPLATE_FILE}" )
	if( APPLE )
		set( OGRE_MEDIA_DIR "Contents/Resources" )
	elseif( ANDROID )
		set( OGRE_MEDIA_DIR "" )
	else()
		set( OGRE_MEDIA_DIR ".." )
	endif()
	configure_file( ${TEMPLATE_FILE} ${OUTPUT_FILE} )
endfunction()

#----------------------------------------------------------------------------------------

function( setupOgreSamplesCommon )
	message( STATUS "Copying OgreSamplesCommon cpp and header files to
		${CMAKE_SOURCE_DIR}/include/OgreCommon
		${CMAKE_SOURCE_DIR}/src/OgreCommon/" )
	include_directories( "${CMAKE_SOURCE_DIR}/include/OgreCommon/" )
	file( COPY "${OGRE_SOURCE}/Samples/2.0/Common/include/"	DESTINATION "${CMAKE_SOURCE_DIR}/include/OgreCommon/" )
	file( COPY "${OGRE_SOURCE}/Samples/2.0/Common/src/"		DESTINATION "${CMAKE_SOURCE_DIR}/src/OgreCommon/" )
endfunction()

#----------------------------------------------------------------------------------------

# Main call to setup Ogre.
macro( setupOgre OGRE_SDK, OGRE_USE_SCENE_FORMAT, OGRE_USE_PLANAR_REFLECTIONS )

	isOgreNext( OGRE_SDK, OGRE_USE_NEW_NAME )
	if( ${OGRE_USE_NEW_NAME} )
		set( OGRE_NEXT "OgreNext" )
		set( OGRE_NEXT_PREFIX "OGRE-Next" )
	else()
		set( OGRE_NEXT "Ogre" )
		set( OGRE_NEXT_PREFIX "OGRE" )
	endif()
	message( STATUS "OgreNext lib name prefix is ${OGRE_NEXT}" )

	set( OGRE_INCLUDE_DIR "${OGRE_SDK}/include/${OGRE_NEXT_PREFIX}" )
	if (WIN32)
		set( OGRE_MEDIA_DIR "${OGRE_SDK}/Media" )
	elseif (APPLE)
		set( OGRE_MEDIA_DIR "${OGRE_SDK}/Media" )
	elseif (UNIX)
		set( OGRE_MEDIA_DIR "${OGRE_SDK}/share/${OGRE_NEXT_PREFIX}/Media" )
	endif()

	if(CMAKE_CONFIGURATION_TYPES)
		link_directories( "${OGRE_SDK}/lib/$<CONFIG>" )
	else()
		link_directories( "${OGRE_SDK}/lib" )
	endif()

	# Parse OgreBuildSettings.h to see if it's a static build
	set( OGRE_DEPENDENCY_LIBS "" )
	file( READ "${OGRE_INCLUDE_DIR}/OgreBuildSettings.h" OGRE_BUILD_SETTINGS_STR )
	string( FIND "${OGRE_BUILD_SETTINGS_STR}" "#define OGRE_BUILD_COMPONENT_ATMOSPHERE" OGRE_BUILD_COMPONENT_ATMOSPHERE )
	string( FIND "${OGRE_BUILD_SETTINGS_STR}" "#define OGRE_STATIC_LIB" OGRE_STATIC )
	if( NOT OGRE_STATIC EQUAL -1 )
		message( STATUS "Detected static build of OgreNext" )
		set( OGRE_STATIC "Static" )

		# Static builds must link against its dependencies
		addStaticDependencies( OGRE_BUILD_SETTINGS_STR )
	else()
		message( STATUS "Detected DLL build of OgreNext" )
		unset( OGRE_STATIC )
	endif()
	findOgreBuildSetting( ${OGRE_BUILD_SETTINGS_STR} OGRE_BUILD_RENDERSYSTEM_GL3PLUS )
	findOgreBuildSetting( ${OGRE_BUILD_SETTINGS_STR} OGRE_BUILD_RENDERSYSTEM_D3D11 )
	findOgreBuildSetting( ${OGRE_BUILD_SETTINGS_STR} OGRE_BUILD_RENDERSYSTEM_METAL )
	findOgreBuildSetting( ${OGRE_BUILD_SETTINGS_STR} OGRE_BUILD_RENDERSYSTEM_VULKAN )
	unset( OGRE_BUILD_SETTINGS_STR )

	if( NOT APPLE )
		# Create debug libraries with _d suffix
		set( OGRE_DEBUG_SUFFIX "_d" )
	endif()

	if( NOT IOS )
		set( CMAKE_PREFIX_PATH "${OGRE_SOURCE}/Dependencies ${CMAKE_PREFIX_PATH}" )
		find_package( SDL2 )
		if( NOT SDL2_FOUND )
			message( "Could not find SDL2. https://www.libsdl.org/" )
		else()
			message( STATUS "Found SDL2" )
			set( OGRE_DEPENDENCY_LIBS ${OGRE_DEPENDENCY_LIBS} ${SDL2_LIBRARY} )
		endif()
	endif()

	if( ${OGRE_USE_SCENE_FORMAT} )
		set( OGRE_LIBRARIES ${OGRE_LIBRARIES}
			debug ${OGRE_NEXT}SceneFormat${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
			optimized ${OGRE_NEXT}SceneFormat${OGRE_STATIC}
			)
	endif()

	if( ${OGRE_USE_PLANAR_REFLECTIONS} )
		set( OGRE_LIBRARIES ${OGRE_LIBRARIES}
			debug ${OGRE_NEXT}PlanarReflections${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
			optimized ${OGRE_NEXT}PlanarReflections${OGRE_STATIC}
			)
	endif()

	if( NOT OGRE_BUILD_COMPONENT_ATMOSPHERE EQUAL -1 )
		message( STATUS "Detected Atmosphere Component. Linking against it." )
		set( OGRE_LIBRARIES ${OGRE_LIBRARIES}
			debug ${OGRE_NEXT}Atmosphere${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
			optimized ${OGRE_NEXT}Atmosphere${OGRE_STATIC}
			)
	endif()

	if( OGRE_STATIC )
		if( OGRE_BUILD_RENDERSYSTEM_D3D11 )
			message( STATUS "Detected D3D11 RenderSystem. Linking against it." )
			set( OGRE_LIBRARIES
				${OGRE_LIBRARIES}
				debug RenderSystem_Direct3D11${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
				optimized RenderSystem_Direct3D11${OGRE_STATIC} )
		endif()
		if( OGRE_BUILD_RENDERSYSTEM_GL3PLUS )
			message( STATUS "Detected GL3+ RenderSystem. Linking against it." )
			set( OGRE_LIBRARIES
				${OGRE_LIBRARIES}
				debug RenderSystem_GL3Plus${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
				optimized RenderSystem_GL3Plus${OGRE_STATIC} )

			if( UNIX )
				set( OGRE_DEPENDENCY_LIBS ${OGRE_DEPENDENCY_LIBS} Xt Xrandr X11 GL )
			endif()
		endif()
		if( OGRE_BUILD_RENDERSYSTEM_METAL )
			message( STATUS "Detected Metal RenderSystem. Linking against it." )
			set( OGRE_LIBRARIES
				${OGRE_LIBRARIES}
				debug RenderSystem_Metal${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
				optimized RenderSystem_Metal${OGRE_STATIC} )
		endif()
		if( OGRE_BUILD_RENDERSYSTEM_VULKAN )
			message( STATUS "Detected Vulkan RenderSystem. Linking against it." )
			set( OGRE_LIBRARIES
				${OGRE_LIBRARIES}
				debug RenderSystem_Vulkan${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
				optimized RenderSystem_Vulkan${OGRE_STATIC} )

			set( OGRE_DEPENDENCY_LIBS ${OGRE_DEPENDENCY_LIBS} xcb X11-xcb xcb-randr )
		endif()
	endif()

	set( OGRE_LIBRARIES
		${OGRE_LIBRARIES}

		debug ${OGRE_NEXT}Overlay${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
		debug ${OGRE_NEXT}HlmsUnlit${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
		debug ${OGRE_NEXT}HlmsPbs${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
		debug ${OGRE_NEXT}Main${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}

		optimized ${OGRE_NEXT}Overlay${OGRE_STATIC}
		optimized ${OGRE_NEXT}HlmsUnlit${OGRE_STATIC}
		optimized ${OGRE_NEXT}HlmsPbs${OGRE_STATIC}
		optimized ${OGRE_NEXT}Main${OGRE_STATIC}

		${OGRE_DEPENDENCY_LIBS}
		)

	# Plugins.cfg
	if( NOT APPLE )
		set( OGRE_PLUGIN_DIR "Plugins" )
	endif()

endmacro()

#----------------------------------------------------------------------------------------

macro( addStaticDependencies OGRE_BUILD_SETTINGS_STR )
	if( WIN32 )
		# Win32 seems to be the only one actually doing Debug builds for Dependencies w/ _d
		set( OGRE_DEP_DEBUG_SUFFIX "_d" )
	endif()

	if( WIN32 )
		set( OGRE_DEPENDENCIES "${OGRE_DEPENDENCIES}/lib/$(ConfigurationName)" CACHE STRING
			 "Path to OGRE-Next's dependencies folder. Only used in Static Builds" )
	elseif( IOS )
		set( OGRE_DEPENDENCIES "${OGRE_DEPENDENCIES}/lib/$(CONFIGURATION)" CACHE STRING
			 "Path to OGRE-Next's dependencies folder. Only used in Static Builds" )
	elseif( APPLE )
		set( OGRE_DEPENDENCIES "${OGRE_DEPENDENCIES}/lib/$(CONFIGURATION)" CACHE STRING
			 "Path to OGRE-Next's dependencies folder. Only used in Static Builds" )
	elseif( ANDROID )
		set( OGRE_DEPENDENCIES "${OGRE_DEPENDENCIES}/lib" CACHE STRING
			"Path to OGRE-Next's dependencies folder. Only used in Static Builds" )
	else()
		set( OGRE_DEPENDENCIES "${OGRE_DEPENDENCIES}/lib" CACHE STRING
			 "Path to OGRE-Next's dependencies folder. Only used in Static Builds" )
	endif()

	string( FIND "${OGRE_BUILD_SETTINGS_STR}" "#define OGRE_NO_FREEIMAGE 0" OGRE_USES_FREEIMAGE )
	if( NOT OGRE_USES_FREEIMAGE EQUAL -1 )
		message( STATUS "Static lib needs FreeImage. Linking against it." )
		set( TMP_DEPENDENCY_LIBS ${TMP_DEPENDENCY_LIBS}
			debug FreeImage${OGRE_DEP_DEBUG_SUFFIX}
			optimized FreeImage )
	endif()

	string( FIND "${OGRE_BUILD_SETTINGS_STR}" "#define OGRE_BUILD_COMPONENT_OVERLAY" OGRE_USES_OVERLAYS )
	if( NOT OGRE_USES_OVERLAYS EQUAL -1 )
		message( STATUS "Static lib needs FreeType. Linking against it." )
		set( TMP_DEPENDENCY_LIBS ${TMP_DEPENDENCY_LIBS}
			debug freetype${OGRE_DEP_DEBUG_SUFFIX}
			optimized freetype )
	endif()

	string( FIND "${OGRE_BUILD_SETTINGS_STR}" "#define OGRE_NO_ZIP_ARCHIVE 0" OGRE_USES_ZIP )
	if( NOT OGRE_USES_ZIP EQUAL -1 )
		message( STATUS "Static lib needs zzip. Linking against it." )
		if( UNIX )
			set( ZZIPNAME zziplib )
		else()
			set( ZZIPNAME zzip )
		endif()
		set( TMP_DEPENDENCY_LIBS ${TMP_DEPENDENCY_LIBS}
			debug ${ZZIPNAME}${OGRE_DEP_DEBUG_SUFFIX}
			optimized ${ZZIPNAME} )
	endif()

	message( STATUS "Static lib needs freetype due to Overlays. Linking against it." )
	set( TMP_DEPENDENCY_LIBS ${TMP_DEPENDENCY_LIBS}
		debug freetype${OGRE_DEP_DEBUG_SUFFIX}
		optimized freetype )

	if( UNIX AND NOT APPLE )
		set( TMP_DEPENDENCY_LIBS ${TMP_DEPENDENCY_LIBS} dl Xt Xrandr X11 xcb Xaw )
	endif()

	set( OGRE_DEPENDENCY_LIBS ${TMP_DEPENDENCY_LIBS} )
endmacro()
