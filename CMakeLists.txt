cmake_minimum_required(VERSION 3.25)

# This tells cmake we have goodies in the /cmake folder
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include (PamplejuceVersion)

# Modern concise way to add dependencies to your project
include (CPM)

# Configures universal binaries and decides which version of macOS to support
include(PamplejuceMacOS)

# Couple tweaks that IMO should be JUCE defaults
include(JUCEDefaults)

# Change me!
# This is the internal name of the project and the name of JUCE's shared code target
# Note: This cannot have spaces (it may be 2024, but you can't have it all!)
# Worry not, JUCE's PRODUCT_NAME can have spaces (and is what DAWs display)
set(PROJECT_NAME "Pamplejuce")

# Worry not, JUCE's PRODUCT_NAME can have spaces (and is what DAWs will display)
# You can also just have it be the same thing as PROJECT_NAME
# You may want to append the major version on the end of this (and PROJECT_NAME) ala:
#   set(PROJECT_NAME "MyPlugin_v${MAJOR_VERSION}")
# Doing so enables major versions to show up in IDEs and DAWs as separate plugins
# allowing you to change parameters and behavior without breaking existing user projects
set(PRODUCT_NAME "Pamplejuce Demo")

# Change me! Used for the MacOS bundle name and Installers
set(COMPANY_NAME "Pamplejuce Company")

# Change me! Used for the MacOS bundle identifier (and signing)
set(BUNDLE_ID "com.pamplejuce.pamplejuce")

# Change me! Set the plugin formats you want built
# Valid choices: AAX Unity VST VST3 AU AUv3 Standalone
set(FORMATS Standalone AU VST3 AUv3)

# Force JUCE to work with sccache
# instruct MSVC to embed debug info (/Z7) instead of emitting a .pdb
set(CMAKE_POLICY_DEFAULT_CMP0141        NEW      CACHE STRING "" FORCE)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded CACHE STRING "" FORCE)

# For simplicity, the name of the CMake project is also the name of the target
project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})

# JUCE is setup as a submodule in the /JUCE folder
# Locally, you must run `git submodule update --init --recursive` once
# and later `git submodule update --remote --merge` to keep it up to date
# On Github Actions, this is done as a part of actions/checkout
add_subdirectory(JUCE)

# Add CLAP format
add_subdirectory(modules/clap-juce-extensions EXCLUDE_FROM_ALL)

# Add any other modules you want modules here, before the juce_add_plugin call
# juce_add_module(modules/my_module)

# This adds the melatonin inspector module
add_subdirectory (modules/melatonin_inspector)

# Build Slang library with Meson
find_program(MESON_EXECUTABLE meson REQUIRED)
set(SLANG_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/Slang")
set(SLANG_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/slang-build")

# Handle macOS universal binary build
if(APPLE AND CMAKE_OSX_ARCHITECTURES)
    # Build separate libraries for each architecture
    set(SLANG_LIBS "")
    foreach(ARCH ${CMAKE_OSX_ARCHITECTURES})
        set(ARCH_BUILD_DIR "${SLANG_BUILD_DIR}-${ARCH}")
        
        # Create a cross-file for Meson
        set(CROSS_FILE "${CMAKE_CURRENT_BINARY_DIR}/meson-${ARCH}.ini")
        file(WRITE "${CROSS_FILE}"
"[binaries]
c = 'clang'
cpp = 'clang++'
ar = 'ar'

[properties]
c_args = ['-arch', '${ARCH}']
cpp_args = ['-arch', '${ARCH}']
c_link_args = ['-arch', '${ARCH}']
cpp_link_args = ['-arch', '${ARCH}']

[host_machine]
system = 'darwin'
cpu_family = '${ARCH}'
cpu = '${ARCH}'
endian = 'little'
")
        
        add_custom_command(
            OUTPUT "${ARCH_BUILD_DIR}/libslang.dylib"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${ARCH_BUILD_DIR}"
            COMMAND ${MESON_EXECUTABLE} setup "${ARCH_BUILD_DIR}" "${SLANG_SOURCE_DIR}" 
                    --buildtype=release --cross-file=${CROSS_FILE}
            COMMAND ${MESON_EXECUTABLE} compile -C "${ARCH_BUILD_DIR}"
            WORKING_DIRECTORY "${SLANG_SOURCE_DIR}"
            COMMENT "Building Slang library for ${ARCH} with Meson"
            VERBATIM
        )
        list(APPEND SLANG_LIBS "${ARCH_BUILD_DIR}/libslang.dylib")
    endforeach()
    
    # Create universal binary using lipo
    add_custom_command(
        OUTPUT "${SLANG_BUILD_DIR}/libslang.dylib"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SLANG_BUILD_DIR}"
        COMMAND lipo -create ${SLANG_LIBS} -output "${SLANG_BUILD_DIR}/libslang.dylib"
        DEPENDS ${SLANG_LIBS}
        COMMENT "Creating universal Slang library"
        VERBATIM
    )
    
    add_custom_target(SlangLib ALL
        DEPENDS "${SLANG_BUILD_DIR}/libslang.dylib"
    )
else()
    # Standard single-architecture build
    add_custom_command(
        OUTPUT "${SLANG_BUILD_DIR}/libslang.so" "${SLANG_BUILD_DIR}/libslang.dylib" "${SLANG_BUILD_DIR}/slang.dll"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SLANG_BUILD_DIR}"
        COMMAND ${MESON_EXECUTABLE} setup "${SLANG_BUILD_DIR}" "${SLANG_SOURCE_DIR}" --buildtype=release
        COMMAND ${MESON_EXECUTABLE} compile -C "${SLANG_BUILD_DIR}"
        WORKING_DIRECTORY "${SLANG_SOURCE_DIR}"
        COMMENT "Building Slang library with Meson"
        VERBATIM
    )

    add_custom_target(SlangLib ALL
        DEPENDS "${SLANG_BUILD_DIR}/libslang.so" "${SLANG_BUILD_DIR}/libslang.dylib" "${SLANG_BUILD_DIR}/slang.dll"
    )
endif()

# Create an imported library target
add_library(slang SHARED IMPORTED GLOBAL)
add_dependencies(slang SlangLib)

# Set library location based on platform
if(APPLE)
    set_target_properties(slang PROPERTIES
        IMPORTED_LOCATION "${SLANG_BUILD_DIR}/libslang.dylib"
    )
elseif(WIN32)
    set_target_properties(slang PROPERTIES
        IMPORTED_LOCATION "${SLANG_BUILD_DIR}/slang.dll"
        IMPORTED_IMPLIB "${SLANG_BUILD_DIR}/slang.lib"
    )
else()
    set_target_properties(slang PROPERTIES
        IMPORTED_LOCATION "${SLANG_BUILD_DIR}/libslang.so"
    )
endif()

# See `docs/CMake API.md` in the JUCE repo for all config options
juce_add_plugin("${PROJECT_NAME}"
    # Icons for the standalone app
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon.png"

    # Change me!
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"

    # On MacOS, plugin is copied to /Users/yourname/Library/Audio/Plug-Ins/
    COPY_PLUGIN_AFTER_BUILD TRUE

    # Change me!
    # A four-character plugin id
    # First character MUST be uppercase for AU formats
    PLUGIN_MANUFACTURER_CODE Pamp

    # Change me!
    # A unique four-character plugin id
    # Note: this must have at least one upper-case character
    PLUGIN_CODE P001
    FORMATS "${FORMATS}"

    # The name of your final executable
    # This is how it's listed in the DAW
    # This can be different from PROJECT_NAME and can have spaces!
    # You might want to use v${MAJOR_VERSION} here once you go to v2...
    PRODUCT_NAME "${PRODUCT_NAME}")

# This lets us use our code in both the JUCE targets and our Test target
# Without running into ODR violations
add_library(SharedCode INTERFACE)

clap_juce_extensions_plugin(TARGET "${PROJECT_NAME}"
    CLAP_ID "${BUNDLE_ID}"
    CLAP_FEATURES audio-effect)

# Enable fast math, C++20 and a few other target defaults
include(SharedCodeDefaults)


# Manually list all .h and .cpp files for the plugin
# If you are like me, you'll use globs for your sanity.
# Just ensure you employ CONFIGURE_DEPENDS so the build system picks up changes
# If you want to appease the CMake gods and avoid globs, manually add files like so:
# set(SourceFiles Source/PluginEditor.h Source/PluginProcessor.h Source/PluginEditor.cpp Source/PluginProcessor.cpp)
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
target_sources(SharedCode INTERFACE ${SourceFiles})

# Add Slang include directory
target_include_directories(SharedCode INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/modules/Slang")

# Adds a BinaryData target for embedding assets into the binary
include(Assets)

# MacOS only: Cleans up folder and target organization on Xcode.
include(XcodePrettify)

# This is where you can set preprocessor definitions for JUCE and your plugin
target_compile_definitions(SharedCode
    INTERFACE

    # JUCE_WEB_BROWSER and JUCE_USE_CURL off by default
    JUCE_WEB_BROWSER=0  # If you set this to 1, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
    JUCE_USE_CURL=0     # If you set this to 1, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
    JUCE_VST3_CAN_REPLACE_VST2=0

    # Uncomment if you are paying for a an Indie/Pro license or releasing under GPLv3
    # JUCE_DISPLAY_SPLASH_SCREEN=0

    # lets the app known if we're Debug or Release
    CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    VERSION="${CURRENT_VERSION}"

    # JucePlugin_Name is for some reason doesn't use the nicer PRODUCT_NAME
    PRODUCT_NAME_WITHOUT_VERSION="Pamplejuce"
)

# Link to any other modules you added (with juce_add_module) here!
# Usually JUCE modules must have PRIVATE visibility
# See https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md#juce_add_module
# However, with Pamplejuce, you'll link modules to SharedCode with INTERFACE visibility
# This allows the JUCE plugin targets and the Tests target to link against it
target_link_libraries(SharedCode
    INTERFACE
    Assets
    melatonin_inspector
    slang
    juce_audio_utils
    juce_audio_processors
    juce_dsp
    juce_gui_basics
    juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Link the JUCE plugin targets our SharedCode target
target_link_libraries("${PROJECT_NAME}" PRIVATE SharedCode)

# Set RPATH so executables can find the Slang library
if(APPLE)
    set_target_properties("${PROJECT_NAME}" PROPERTIES
        BUILD_RPATH "${SLANG_BUILD_DIR}"
        INSTALL_RPATH "@executable_path/../Frameworks;@loader_path/../Frameworks"
    )
    
    # Copy Slang library into each plugin format bundle after build
    foreach(FORMAT ${FORMATS})
        if(FORMAT STREQUAL "VST3")
            set(TARGET_NAME "${PROJECT_NAME}_VST3")
        elseif(FORMAT STREQUAL "AU")
            set(TARGET_NAME "${PROJECT_NAME}_AU")
        elseif(FORMAT STREQUAL "AUv3")
            set(TARGET_NAME "${PROJECT_NAME}_AUv3")
        elseif(FORMAT STREQUAL "Standalone")
            set(TARGET_NAME "${PROJECT_NAME}_Standalone")
        elseif(FORMAT STREQUAL "AAX")
            set(TARGET_NAME "${PROJECT_NAME}_AAX")
        else()
            continue()
        endif()
        
        if(TARGET ${TARGET_NAME})
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>/Contents/Frameworks"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                        "${SLANG_BUILD_DIR}/libslang.dylib" 
                        "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>/Contents/Frameworks/"
                COMMAND install_name_tool -change "@rpath/libslang.dylib" 
                        "@loader_path/../Frameworks/libslang.dylib"
                        "$<TARGET_FILE:${TARGET_NAME}>"
                COMMENT "Copying Slang library into ${TARGET_NAME} bundle"
            )
        endif()
    endforeach()
    
    # Also handle CLAP if it exists
    if(TARGET "${PROJECT_NAME}_CLAP")
        add_custom_command(TARGET "${PROJECT_NAME}_CLAP" POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_CLAP>/Contents/Frameworks"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                    "${SLANG_BUILD_DIR}/libslang.dylib" 
                    "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_CLAP>/Contents/Frameworks/"
            COMMAND install_name_tool -change "@rpath/libslang.dylib" 
                    "@loader_path/../Frameworks/libslang.dylib"
                    "$<TARGET_FILE:${PROJECT_NAME}_CLAP>"
            COMMENT "Copying Slang library into CLAP bundle"
        )
    endif()
elseif(UNIX)
    set_target_properties("${PROJECT_NAME}" PROPERTIES
        BUILD_RPATH "${SLANG_BUILD_DIR}"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib"
    )
endif()

# IPP support, comment out to disable
include(PamplejuceIPP)

# Everything related to the tests target
include(Tests)

# Set RPATH for Tests executable to find Slang library
if(TARGET Tests)
    if(APPLE)
        set_target_properties(Tests PROPERTIES
            BUILD_RPATH "${SLANG_BUILD_DIR}"
            INSTALL_RPATH "@executable_path;@loader_path"
        )
    elseif(UNIX)
        set_target_properties(Tests PROPERTIES
            BUILD_RPATH "${SLANG_BUILD_DIR}"
            INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib"
        )
    endif()
endif()

# A separate target for Benchmarks (keeps the Tests target fast)
include(Benchmarks)

# Set RPATH for Benchmarks executable to find Slang library
if(TARGET Benchmarks)
    if(APPLE)
        set_target_properties(Benchmarks PROPERTIES
            BUILD_RPATH "${SLANG_BUILD_DIR}"
            INSTALL_RPATH "@executable_path;@loader_path"
        )
    elseif(UNIX)
        set_target_properties(Benchmarks PROPERTIES
            BUILD_RPATH "${SLANG_BUILD_DIR}"
            INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib"
        )
    endif()
endif()

# Output some config for CI (like our PRODUCT_NAME)
include(GitHubENV)
