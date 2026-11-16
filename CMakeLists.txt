cmake_minimum_required(VERSION 3.22)
project(FoundationBass VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(BASS_DSP_ONLY "Build portable DSP tests without JUCE" OFF)
include(CTest)
add_executable(BassDspTests Tests/DspTests.cpp)
target_include_directories(BassDspTests PRIVATE Source)
add_test(NAME BassDSP COMMAND BassDspTests)
if(BASS_DSP_ONLY)
  return()
endif()
set(JUCE_DIR "" CACHE PATH "Existing JUCE 8.0.6 source directory")
if(JUCE_DIR)
  add_subdirectory("${JUCE_DIR}" juce)
else()
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 51a8a6d7aeae7326956d747737ccf1575e61e209 GIT_SHALLOW FALSE)
  FetchContent_MakeAvailable(JUCE)
endif()
juce_add_plugin(FoundationBass
  COMPANY_NAME "Aulamix" BUNDLE_ID "com.aulamix.foundationbass"
  PLUGIN_MANUFACTURER_CODE Aumx PLUGIN_CODE Fb01
  PRODUCT_NAME "Foundation Bass" FORMATS VST3 Standalone
  IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE VST3_CATEGORIES Fx Dynamics EQ Distortion)
target_sources(FoundationBass PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(FoundationBass PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(FoundationBass PRIVATE juce::juce_audio_utils
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
add_executable(BassProcessorTests Tests/ProcessorTests.cpp)
target_include_directories(BassProcessorTests PRIVATE Source $<TARGET_PROPERTY:FoundationBass,INCLUDE_DIRECTORIES>)
target_compile_definitions(BassProcessorTests PRIVATE $<TARGET_PROPERTY:FoundationBass,COMPILE_DEFINITIONS>)
target_link_libraries(BassProcessorTests PRIVATE FoundationBass)
add_test(NAME BassProcessor COMMAND BassProcessorTests)
# The definitive asset is optional until supplied. No image generator or conversion runs here.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/definitive.png")
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/asset.py" verify
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" COMMAND_ERROR_IS_FATAL ANY)
  juce_add_binary_data(BassArtwork SOURCES assets/definitive.png)
  target_link_libraries(FoundationBass PRIVATE BassArtwork)
  target_compile_definitions(FoundationBass PRIVATE BASS_HAS_ARTWORK=1)
endif()
