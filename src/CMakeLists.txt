set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(SOURCE_DIR "${ROOT_DIR}/src")
set(SOURCE_FILES
	"${SOURCE_DIR}/PCH.h"
	"${SOURCE_DIR}/main.cpp"
	"${SOURCE_DIR}/AttackState.cpp"
	"${SOURCE_DIR}/AttackState.h"
	"${SOURCE_DIR}/ConfigManager.cpp"
	"${SOURCE_DIR}/ConfigManager.h"
	"${SOURCE_DIR}/Settings.h"
	"${SOURCE_DIR}/InputInterceptor.cpp"
	"${SOURCE_DIR}/InputInterceptor.h"
	"${SOURCE_DIR}/utils.cpp"
	"${SOURCE_DIR}/utils.h"
	"${SOURCE_DIR}/ActionAllowedHook.cpp"
	"${SOURCE_DIR}/ActionAllowedHook.h"
	"${SOURCE_DIR}/sksevr_api.h"
	"${SOURCE_DIR}/haptics.cpp"
	"${SOURCE_DIR}/haptics.h"
	"${SOURCE_DIR}/SpellChargeTracker.cpp"
	"${SOURCE_DIR}/SpellChargeTracker.h"

)


source_group(TREE "${ROOT_DIR}" FILES ${SOURCE_FILES})

set(VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/Plugin.h")
configure_file(
	"${ROOT_DIR}/cmake/Plugin.h.in"
	"${VERSION_HEADER}"
	@ONLY
)

source_group("src" FILES "${VERSION_HEADER}")

configure_file(
	"${ROOT_DIR}/cmake/version.rc.in"
	"${CMAKE_CURRENT_BINARY_DIR}/version.rc"
	@ONLY
)

add_library(
	"${PROJECT_NAME}"
	SHARED
	${SOURCE_FILES}
	"${VERSION_HEADER}"
	"${CMAKE_CURRENT_BINARY_DIR}/version.rc"
	"${ROOT_DIR}/.clang-format"
	"${ROOT_DIR}/.editorconfig"
)

target_compile_features(
	"${PROJECT_NAME}"
	PRIVATE
		cxx_std_20
)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	target_compile_options(
		"${PROJECT_NAME}"
		PRIVATE
			"/sdl"	# Enable Additional Security Checks
			"/utf-8"	# Set Source and Executable character sets to UTF-8
			"/Zi"	# Debug Information Format

			"/permissive-"	# Standards conformance
			"/Zc:preprocessor"	# Enable preprocessor conformance mode

			"/wd4200" # nonstandard extension used : zero-sized array in struct/union

			"$<$<CONFIG:DEBUG>:>"
			"$<$<CONFIG:RELEASE>:/Zc:inline;/JMC-;/Ob3>"
	)

	target_link_options(
		"${PROJECT_NAME}"
		PRIVATE
			"$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
			"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
	)
endif()

target_include_directories(
	"${PROJECT_NAME}"
	PRIVATE
		"${CMAKE_CURRENT_BINARY_DIR}/src"
		"${SOURCE_DIR}"
)


add_subdirectory("${ROOT_DIR}/external/CommonLibVR" CommonLibVR EXCLUDE_FROM_ALL)

target_link_libraries(
	"${PROJECT_NAME}"
	PRIVATE
		CommonLibVR::CommonLibVR
		"${ROOT_DIR}/external/CommonLibVR/extern/openvr/lib/win64/openvr_api.lib"
)

target_precompile_headers(
	"${PROJECT_NAME}"
	PRIVATE
		"${SOURCE_DIR}/PCH.h"
)

install(
	FILES
		"$<TARGET_FILE:${PROJECT_NAME}>"
	DESTINATION "SKSE/Plugins"
	COMPONENT "main"
)

install(
	FILES
		"$<TARGET_PDB_FILE:${PROJECT_NAME}>"
	DESTINATION "/"
	COMPONENT "pdbs"
)

# if("${COPY_OUTPUT}")
add_custom_command(
	TARGET "${PROJECT_NAME}"
	POST_BUILD
	COMMAND "${PROJECT_SOURCE_DIR}/scripts/kill_skyrimvr.bat"
	#COMMAND "${PROJECT_SOURCE_DIR}/scripts/compile_papyrus_mo2.bat"
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${MO2Path}/overwrite/SKSE/Plugins/"
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${MO2Path}/overwrite/SKSE/Plugins/"
	COMMAND "${PROJECT_SOURCE_DIR}/scripts/start_skyrimvr.bat"
	VERBATIM
)

# endif()
add_compile_definitions(SKYRIMVR)

