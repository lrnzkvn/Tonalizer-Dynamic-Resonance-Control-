cmake_minimum_required(VERSION 3.22)
project(Tonalizer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory("C:/JUCE" "${CMAKE_CURRENT_BINARY_DIR}/JUCE")

juce_add_plugin(Tonalizer
    COMPANY_NAME "Cleanser Audio"
    PLUGIN_MANUFACTURER_CODE Clnr
    PLUGIN_CODE Tonl
    FORMATS VST3 LV2 Standalone
    PRODUCT_NAME "Tonalizer"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Dynamics
    LV2URI "https://cleanseraudio.com/plugins/tonalizer"
)

juce_add_binary_data(TonalizerAssets
    SOURCES
        assets/main_logo.png
        assets/reference_gui.png
)

set(VOID_CORE "${CMAKE_CURRENT_SOURCE_DIR}/../anythingelse/VoidCore")

target_sources(Tonalizer PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/dsp/DetectionEngine.cpp
    src/dsp/BandOverlay.cpp
    src/dsp/SpectralEraser.cpp
    src/dsp/PitchDetector.cpp
    src/dsp/SpectralMorpher.cpp
    src/model/MusicalScale.cpp
    src/model/TonalClassifier.cpp
    src/model/Parameters.cpp
    src/ui/LookAndFeel.cpp
    src/ui/DotMatrixDisplay.cpp
    src/ui/KeyScaleSelector.cpp
    src/ui/EraseKnob.cpp
    ${VOID_CORE}/VoidLookAndFeel.cpp
)

target_include_directories(Tonalizer PRIVATE src ${VOID_CORE})

target_compile_definitions(Tonalizer PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(Tonalizer
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        TonalizerAssets
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
