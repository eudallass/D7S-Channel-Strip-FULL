cmake_minimum_required(VERSION 3.22)

project(D7SChannelStripFull VERSION 0.1.4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(D7SChannelStripFull
    COMPANY_NAME "Dallass"
    BUNDLE_ID "com.dallass.d7schannelstripfull"
    PLUGIN_MANUFACTURER_CODE Dals
    PLUGIN_CODE D7sf
    FORMATS VST3 Standalone
    PRODUCT_NAME "D7S Channel Strip FULL"
    PLUGIN_CATEGORY "Fx"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_AUTO_MANIFEST TRUE
)

target_sources(D7SChannelStripFull
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(D7SChannelStripFull
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(D7SChannelStripFull
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
