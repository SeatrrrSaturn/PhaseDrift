cmake_minimum_required(VERSION 3.22)

project(PhaseDrift VERSION 0.9.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)

# --------------------------------------------------------------------------------
# JUCE
# Look for a local checkout first (faster), otherwise fetch it with FetchContent.
# --------------------------------------------------------------------------------
set(JUCE_LOCAL "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" CACHE PATH "Local JUCE checkout")
if(NOT EXISTS "${JUCE_LOCAL}/CMakeLists.txt")
    set(JUCE_LOCAL "${CMAKE_CURRENT_SOURCE_DIR}/../_deps/juce")
endif()

if(EXISTS "${JUCE_LOCAL}/CMakeLists.txt")
    message(STATUS "Using local JUCE at ${JUCE_LOCAL}")
    add_subdirectory("${JUCE_LOCAL}" JUCE)
else()
    message(STATUS "JUCE not found locally - fetching from GitHub")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.10
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# --------------------------------------------------------------------------------
# Plugin target
# --------------------------------------------------------------------------------
juce_add_plugin(PhaseDrift
    COMPANY_NAME              "PhaseDrift Audio"
    COMPANY_WEBSITE           "https://github.com/"
    BUNDLE_ID                 "com.phaseDrift.audio"
    PLUGIN_MANUFACTURER_CODE  "PhDr"
    PLUGIN_CODE               "PhDs"
    PRODUCT_NAME              "Phase Drift"
    PLUGIN_NAME               "Phase Drift"
    FORMATS                   VST3
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    VST3_CATEGORIES           "Fx"
    DESCRIPTION               "Experimental phase-delay processor"
)

target_sources(PhaseDrift PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/PhaseLookAndFeel.cpp
)

target_compile_definitions(PhaseDrift PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_WINRT_MIDI=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_link_libraries(PhaseDrift PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
PUBLIC
    juce::juce_recommended_config_flags
)

if(MSVC)
    target_link_libraries(PhaseDrift PRIVATE juce::juce_recommended_lto_flags)
endif()

# --------------------------------------------------------------------------------
# The built bundle ends up in:
#   build/PhaseDrift_artefacts/Release/VST3/Phase Drift.vst3
# --------------------------------------------------------------------------------
