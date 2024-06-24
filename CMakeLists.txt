cmake_minimum_required(VERSION 3.21)
message("Using toolchain file ${CMAKE_TOOLCHAIN_FILE}.")
########################################################################################################################
## Define project
########################################################################################################################
project(
        Ammo_Patcher
        VERSION 1.0.8
        DESCRIPTION "A simple SKSE plugin built with CommonLibSSE to patch AMMO"
        LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)
set(CMAKE_OPTIMIZE_DEPENDENCIES ON)

include(GNUInstallDirs)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        @ONLY)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
        cmake_policy(SET CMP0135 NEW)
endif()

# Add headers
file(
  GLOB_RECURSE
  headers
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)
list(REMOVE_ITEM headers ${CMAKE_CURRENT_SOURCE_DIR}/include/PCH.h)
list(REMOVE_ITEM headers ${CMAKE_CURRENT_SOURCE_DIR}/include/a.h)
list(REMOVE_ITEM headers ${CMAKE_CURRENT_SOURCE_DIR}/include/s.h)

# Add sources
file(
  GLOB_RECURSE
  sources
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
  ${CMAKE_CURRENT_BINARY_DIR}/version.rc
)

set(tests )

set(misc
        ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/CMakePresets.json
        ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg-configuration.json
        ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json
        ${CMAKE_CURRENT_SOURCE_DIR}/.clang-format)

source_group(
        TREE ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        ${headers}
        ${sources}
        ${tests}
        ${misc})

#########################################################################################################################
### Build options
#########################################################################################################################
message("Options:")
option(BUILD_TESTS "Build unit tests." OFF)
message("\tTests: ${BUILD_TESTS}")

########################################################################################################################
## Configure target DLL
########################################################################################################################
set(PROJECT_AUTHOR "NoName365")

# add_subdirectory($ENV{CommonLibSSEPath} CommonLibSSE-NG)
# include($ENV{CommonLibSSEPath}/cmake/CommonLibSSE.cmake)
find_package(CommonLibSSE CONFIG REQUIRED)
include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${headers} ${sources} ${misc} AUTHOR ${PROJECT_AUTHOR})
add_library("${PROJECT_NAME}::${PROJECT_NAME}" ALIAS "${PROJECT_NAME}")
# Ensure 7z is available
find_program(SEVEN_ZIP_EXECUTABLE NAMES 7z 7za)

if(NOT SEVEN_ZIP_EXECUTABLE)
    message(FATAL_ERROR "7-Zip (7z or 7za) not found. Please install 7-Zip.")
endif()

target_include_directories(${PROJECT_NAME}
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>
        $<INSTALL_INTERFACE:src>
        nlohmann_json::nlohmann_json)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_precompile_headers(${PROJECT_NAME}
        PRIVATE
        include/PCH.h)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(TARGETS ${PROJECT_NAME}
        DESTINATION "${CMAKE_INSTALL_LIBDIR}")

#adding a define for NDEBUG for future uses(if any)
if(${CMAKE_BUILD_TYPE} STREQUAL Debug OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
    add_definitions(-DNDEBUG)
endif()


########################################################################################################################
## Automatic plugin deployment
########################################################################################################################
# Automatic deployment to FOMOD directory.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BUILD_NAME "Debug")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(BUILD_NAME "Release")
        elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            set(BUILD_NAME "RelWithDebInfo")
                elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
                    set(BUILD_NAME "MinSizeRel")
endif()
message("Build Type : ${CMAKE_BUILD_TYPE}")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Plugin - ${BUILD_NAME}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Plugin - ${BUILD_NAME}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Configuration/${PROJECT_NAME}.json" "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Common"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Configuration/ExclusionHint.json" "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Common"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Configuration/MainHint.json" "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Common")

if(${CMAKE_BUILD_TYPE} STREQUAL Debug OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PDB_FILE:${PROJECT_NAME}> "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Plugin - ${BUILD_NAME}/")
endif()
file(GLOB_RECURSE OUTPUT_DLLS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Plugin - ${BUILD_NAME}/${PROJECT_NAME}.dll")
if(${CMAKE_BUILD_TYPE} STREQUAL Debug OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
    file(GLOB_RECURSE OUTPUT_PDBS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Plugin - ${BUILD_NAME}/${PROJECT_NAME}.pdb")
endif()
file(GLOB_RECURSE OUTPUT_JSONS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Common/**.json")
set_property(TARGET ${PROJECT_NAME}
        APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${OUTPUT_DLLS}"
		APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${OUTPUT_JSONS}")
if(${CMAKE_BUILD_TYPE} STREQUAL Debug OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
    set_property(TARGET ${PROJECT_NAME}
            APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${OUTPUT_PDBS}")
endif()

# Automatic deployment to Mod Organizer 2 mod directory.
foreach (DEPLOY_TARGET $ENV{SkyrimPluginTargets})    #set your "SkyrimPluginTargets" environment variable to your MO2's Mods Folder
        message("Adding deployment target ${DEPLOY_TARGET}.")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEPLOY_TARGET}/${PROJECT_NAME} - ${CMAKE_BUILD_TYPE}/SKSE/Plugins/"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEPLOY_TARGET}/${PROJECT_NAME} - ${CMAKE_BUILD_TYPE}/SKSE/Plugins/Ammo Patcher Hints/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Configuration/ExclusionHint.json" "${DEPLOY_TARGET}/${PROJECT_NAME} - ${CMAKE_BUILD_TYPE}/SKSE/Plugins/Ammo Patcher Hints/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Configuration/MainHint.json" "${DEPLOY_TARGET}/${PROJECT_NAME} - ${CMAKE_BUILD_TYPE}/SKSE/Plugins/Ammo Patcher Hints/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> "${DEPLOY_TARGET}/${PROJECT_NAME} - ${CMAKE_BUILD_TYPE}/SKSE/Plugins/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Configuration/${PROJECT_NAME}.json" "${DEPLOY_TARGET}/${PROJECT_NAME} - ${CMAKE_BUILD_TYPE}/SKSE/Plugins/"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEPLOY_TARGET}/Common For ${PROJECT_NAME}/SKSE/Plugins/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Configuration/My Config/${PROJECT_NAME}.json" "${DEPLOY_TARGET}/Common For ${PROJECT_NAME}/SKSE/Plugins/")
        if(${CMAKE_BUILD_TYPE} STREQUAL Debug OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PDB_FILE:${PROJECT_NAME}> "${DEPLOY_TARGET}/${PROJECT_NAME} - ${CMAKE_BUILD_TYPE}/SKSE/Plugins/")
        endif()
endforeach()


set(ZIP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution")
set(OUTPUT_LOG "${CMAKE_CURRENT_SOURCE_DIR}/7z_output.log")
set(TARGET_ZIP "${PROJECT_NAME}_${PROJECT_VERSION}.7z")

ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${SEVEN_ZIP_EXECUTABLE} a -t7z -mx=9 "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Artifact/${TARGET_ZIP}" "${ZIP_DIR}/*" > ${OUTPUT_LOG}
)

set(FOMOD_INFO_DATA 
"<fomod> \n"
"	<Name>${PROJECT_NAME}</Name> \n"
"	<Author>${PROJECT_AUTHOR}</Author> \n"
"	<Version>${PROJECT_VERSION}</Version> \n"
"	<Website>https://www.nexusmods.com/skyrimspecialedition/mods/109061/</Website> \n"
"	<Description>\n"
"A simple SKSE plugin to patch All AMMO.\n"
"You can change the speed,gravity of arrows and bolts separately.\n"
"You can also limit the speed of arrows and/or bolts between a maximum and a minimum value.\n"
"You can also optionally have practically infinite AMMO for player or followers or both.(adds one arrow or bolt when shot, you will get a notification)\n</Description> \n"
"	<Groups>\n"
"		<element>Ammo</element>\n"
"	</Groups> \n" 
"</fomod> \n"
)

if(NOT DEFINED ENV{GITHUB_ENV})
    file(WRITE "${ZIP_DIR}/fomod/info.xml" ${FOMOD_INFO_DATA})
endif()


# Define the workflow file path
set(WORKFLOW_FILE "${CMAKE_SOURCE_DIR}/.github/workflows/CMake SKSE Setup.yml")

set(WORKFLOW_FILE_DATA 
    "name: CMake SKSE Setup\n"
    "\n"
    "on:\n"
    "  push:\n"
    "    branches: [ \"main\" ]\n"
    "\n"
    "env:\n"
    "  PROJECT_NAME: ${PROJECT_NAME}\n"
    "  PROJECT_VERSION: ${PROJECT_VERSION}\n"
    "\n"
    "\n"
    "jobs:\n"
    "  build:\n"
    "    runs-on: windows-latest\n"
    "    permissions:\n"
    "      contents: write\n"
    "\n"
    "    steps:\n"
    "    - uses: actions/checkout@v4\n"
    "\n"
    "    - name: Setup cmake\n"
    "      uses: lukka/get-cmake@latest\n"
    "\n"
    "    - name: Restore from cache and setup vcpkg executable and data files.\n"
    "      uses: lukka/run-vcpkg@v11\n"
    "      with:\n"
    "        vcpkgJsonGlob: '$\{\{ github.workspace \}\}/vcpkg.json'\n"
    "\n"
    "    - name: Configure and Run CMake Preset for \"Debug\" preset\n"
    "      uses: lukka/run-cmake@v10\n"
    "      with:\n"
    "        configurePreset: 'Debug'\n"
    "        buildPreset: 'Debug'\n"
    "\n"
    "    - name: Configure and Run CMake Preset for \"Release\" preset\n"
    "      uses: lukka/run-cmake@v10\n"
    "      with:\n"
    "        configurePreset: 'Release'\n"
    "        buildPreset: 'Release'\n"
    "\n"
    "    - name: Deploy Aritfact\n"
    "      uses: actions/upload-artifact@v4.3.3\n"
    "      with:\n"
    "        path: $\{\{ github.workspace \}\}/contrib/Artifact/$\{\{ env.PROJECT_NAME \}\}_$\{\{ env.PROJECT_VERSION \}\}.7z\n"
    "\n"
    "    - name: Upload files to a GitHub release\n"
    "      uses: ncipollo/release-action@v1.14.0\n"
    "      with:\n"
    "        token: $\{\{ secrets.GITHUB_TOKEN \}\}\n"
    "        artifacts: $\{\{ github.workspace \}\}/contrib/Artifact/$\{\{ env.PROJECT_NAME \}\}_$\{\{ env.PROJECT_VERSION \}\}.7z\n"
    "        tag: $\{\{ env.PROJECT_VERSION \}\}\n"
    "        commit: main\n"
    "        generateReleaseNotes: true\n"
    "        artifactErrorsFailBuild: true\n"
    "        makeLatest: \"latest\"\n"
    "        name: $\{\{ env.PROJECT_NAME \}\} $\{\{ env.PROJECT_VERSION \}\}\n"
)

# Check if the GitHub environment variable is defined
if(DEFINED ENV{GITHUB_ENV})
    message(STATUS "GitHub environment detected. Skipping workflow modification.")
else()
    message(STATUS "GitHub environment not detected. Executing workflow modification.")
    file(WRITE ${WORKFLOW_FILE} ${WORKFLOW_FILE_DATA})
endif()
