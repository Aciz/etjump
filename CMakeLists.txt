cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(etjump VERSION 3.2.2 HOMEPAGE_URL "etjump.com" LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 14)

option(BUILD_TESTS "Enable tests building (ON by default)" ON)

set(BUNDLED_TARGETS_FOLDER Bundled)
set(PACKING_TARGETS_FOLDER Package)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER CMake)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(PlatformDetails)
include(CompilerOptions)
include(VersionDescription)
include(SourceGroup)

get_version_description(GAME_VERSION)

set(BASE_DIR "${CMAKE_PROJECT_NAME}")
set(BASE_DIR_PATH "${CMAKE_BINARY_DIR}/${BASE_DIR}")
file(MAKE_DIRECTORY "${BASE_DIR_PATH}")

create_compiler_opts(cxx_compiler_opts WARN 3 DEFINE 
	GAME_NAME="${CMAKE_PROJECT_NAME}" 
	GAME_URL="${CMAKE_PROJECT_HOMEPAGE_URL}" 
	GAME_VERSION="${GAME_VERSION}"
	GIT_COMMIT_HASH="${GAME_VERSION_COMMIT_HASH}"
	GIT_COMMIT_HASH_SHORT="${GAME_VERSION_COMMIT_HASH_SHORT}"
)
create_compiler_opts(cxx_compiler_opts_w0 WARN 0)

# need to re-set these again for git_version.h since it can't read compiler opts
# we still need to set compiler opts for the stuff in bg_public.h though
set(GAME_NAME "${CMAKE_PROJECT_NAME}")
set(GAME_URL "${CMAKE_PROJECT_HOMEPAGE_URL}")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/git_version.h.in"
		"${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/git_version.h" @ONLY)

add_subdirectory(deps/json)
add_subdirectory(deps/sha-1)
add_subdirectory(deps/sqlite)
add_subdirectory(deps/sqlite_modern_cpp)
add_subdirectory(deps/gtest)
add_subdirectory(deps/uuid4)

# fmt 10.0.0 changed default of FMT_INSTALL from OFF to ON
set(FMT_INSTALL OFF)
add_subdirectory(deps/fmt EXCLUDE_FROM_ALL)

target_link_libraries(libjson PRIVATE cxx_compiler_opts_w0)
target_link_libraries(libsha1 PRIVATE cxx_compiler_opts_w0)
target_link_libraries(libuuid4 PRIVATE cxx_compiler_opts_w0)
target_link_libraries(libsqlite PRIVATE cxx_compiler_opts_w0)
target_link_libraries(fmt PRIVATE cxx_compiler_opts_w0)
target_link_libraries(libsqlite_modern_cpp INTERFACE cxx_compiler_opts_w0)

set_target_properties(gmock gmock_main gtest gtest_main libjson libsha1 libsqlite libsqlite_modern_cpp libuuid4 fmt
                      PROPERTIES FOLDER ${BUNDLED_TARGETS_FOLDER})

add_subdirectory(src/cgame)
add_subdirectory(src/game)
add_subdirectory(src/ui)
add_subdirectory(assets)

set_target_properties(mod_pk3 mod_release 
                      PROPERTIES FOLDER ${PACKING_TARGETS_FOLDER})

# set cgame as startup project in Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT cgame)

if (BUILD_TESTS)
	message(STATUS "Enabling tests building -- done")
	enable_testing()
	add_subdirectory(tests)
endif()
