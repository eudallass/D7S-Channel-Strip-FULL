#include "../PluginProcessor.h"

juce::AudioProcessorValueTreeState::ParameterLayout D7SChannelStripFullAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "master_bypass", 1 }, "Master Bypass", false, juce::AudioParameterBoolAttributes().withAutomatable (true).withMeta (true)));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "rack_input", 1 }, "Rack Input", juce::NormalisableRange<float> (-24.0f, 24.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "rack_output", 1 }, "Rack Output", juce::NormalisableRange<float> (-24.0f, 24.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "rack_mix", 1 }, "Rack Mix", juce::NormalisableRange<float> (0.0f, 100.0f), 100.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "noisegt1_suppression", 1 }, "NoiseGT1 Suppression", juce::NormalisableRange<float> (0.0f, 100.0f), 50.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "noisegt1_bypass", 1 }, "NoiseGT1 Bypass", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_hpf", 1 }, "EQ 4K HPF", juce::NormalisableRange<float> (20.0f, 500.0f), 20.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_lpf", 1 }, "EQ 4K LPF", juce::NormalisableRange<float> (4000.0f, 22000.0f), 22000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_lf_freq", 1 }, "EQ 4K LF Freq", juce::NormalisableRange<float> (30.0f, 450.0f), 80.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_lf_gain", 1 }, "EQ 4K LF Gain", juce::NormalisableRange<float> (-15.0f, 15.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "eq4k_lf_bell", 1 }, "EQ 4K LF Bell", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_lmf_freq", 1 }, "EQ 4K LMF Freq", juce::NormalisableRange<float> (200.0f, 2500.0f), 600.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_lmf_gain", 1 }, "EQ 4K LMF Gain", juce::NormalisableRange<float> (-15.0f, 15.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_lmf_q", 1 }, "EQ 4K LMF Q", juce::NormalisableRange<float> (0.4f, 4.0f), 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_hmf_freq", 1 }, "EQ 4K HMF Freq", juce::NormalisableRange<float> (600.0f, 7000.0f), 3000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_hmf_gain", 1 }, "EQ 4K HMF Gain", juce::NormalisableRange<float> (-15.0f, 15.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_hmf_q", 1 }, "EQ 4K HMF Q", juce::NormalisableRange<float> (0.4f, 4.0f), 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_hf_freq", 1 }, "EQ 4K HF Freq", juce::NormalisableRange<float> (1500.0f, 16000.0f), 10000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_hf_gain", 1 }, "EQ 4K HF Gain", juce::NormalisableRange<float> (-15.0f, 15.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "eq4k_hf_bell", 1 }, "EQ 4K HF Bell", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_drive", 1 }, "EQ 4K Drive", juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "eq4k_bypass", 1 }, "EQ 4K Bypass", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp76_input", 1 }, "76 Input", juce::NormalisableRange<float> (0.0f, 10.0f), 4.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp76_output", 1 }, "76 Output", juce::NormalisableRange<float> (0.0f, 10.0f), 5.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp76_attack", 1 }, "76 Attack", juce::NormalisableRange<float> (1.0f, 7.0f), 3.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp76_release", 1 }, "76 Release", juce::NormalisableRange<float> (1.0f, 7.0f), 5.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "comp76_ratio", 1 }, "76 Ratio", juce::StringArray { "4", "8", "12", "20", "All" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "comp76_sc_hpf", 1 }, "76 SC HPF", juce::StringArray { "Off", "60", "90", "150" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "comp76_bypass", 1 }, "76 Bypass", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp2a_peak", 1 }, "2A Peak Reduction", juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp2a_gain", 1 }, "2A Gain", juce::NormalisableRange<float> (0.0f, 100.0f), 40.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "comp2a_mode", 1 }, "2A Mode", juce::StringArray { "Compress", "Limit" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp2a_emphasis", 1 }, "2A HF Emphasis", juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp2a_mix", 1 }, "2A Mix", juce::NormalisableRange<float> (0.0f, 100.0f), 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "comp2a_sc_hpf", 1 }, "2A SC HPF", juce::StringArray { "Off", "60", "90", "150" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "comp2a_bypass", 1 }, "2A Bypass", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "tube_beauty", 1 }, "Tube Beauty", juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "tube_beast", 1 }, "Tube Beast", juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "tube_sensitivity", 1 }, "Tube Sensitivity", juce::NormalisableRange<float> (0.0f, 100.0f), 50.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "tube_mix", 1 }, "Tube Mix", juce::NormalisableRange<float> (0.0f, 100.0f), 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "tube_bypass", 1 }, "Tube Bypass", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "clipper_threshold", 1 }, "Clipper Threshold", juce::NormalisableRange<float> (-30.0f, 0.0f), -4.4f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "clipper_pre",       1 }, "Clipper Pre Gain", juce::NormalisableRange<float> (-12.0f, 24.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "clipper_post",      1 }, "Clipper Post Gain", juce::NormalisableRange<float> (-24.0f, 24.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "clipper_bypass",    1 }, "Clipper Bypass", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "esser_threshold", 1 }, "Esser Threshold", juce::NormalisableRange<float> (-60.0f, 0.0f), -24.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "esser_freq", 1 }, "Esser Frequency", juce::NormalisableRange<float> (500.0f, 20000.0f), 7000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "esser_range", 1 }, "Esser Range", juce::NormalisableRange<float> (0.0f, 24.0f), 12.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "esser_mode", 1 }, "Esser Mode", juce::StringArray { "Wide", "Split" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "esser_bypass", 1 }, "Esser Bypass", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "delay_mix", 1 }, "Delay Mix", juce::NormalisableRange<float> (0.0f, 100.0f), 25.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "delay_feedback", 1 }, "Delay Feedback", juce::NormalisableRange<float> (0.0f, 100.0f), 35.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "delay_time", 1 }, "Delay Time Legacy", juce::StringArray { "1/32", "1/16", "1/8", "1/4", "1/2", "1/1", "1/8T", "1/4T", "1/8D", "1/4D" }, 3));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "delay_mode", 1 }, "Delay Mode", juce::StringArray { "Msec", "Note", "Dotted", "Triplet" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "delay_fraction_index", 1 }, "Delay Fraction Index", juce::NormalisableRange<float> (0.0f, 6.0f, 1.0f), 2.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "delay_time_ms", 1 }, "Delay Time ms", juce::NormalisableRange<float> (0.1f, 3000.0f, 0.1f, 0.35f), 250.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "delay_bypass", 1 }, "Delay Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "delay_glide_on", 1 }, "Delay Glide", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "delay_glide_direction", 1 }, "Delay Glide Direction", juce::StringArray { "Up", "Down", "Random" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "delay_glide_time", 1 }, "Delay Glide Time", juce::NormalisableRange<float> (0.0f, 100.0f), 35.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "analyzer_pre_enabled", 1 }, "Analyzer Pre", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "analyzer_post_enabled", 1 }, "Analyzer Post", true));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "analyzer_resolution", 1 }, "Analyzer Resolution", juce::StringArray { "Low", "Medium", "High", "Maximum" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "analyzer_speed", 1 }, "Analyzer Speed", juce::StringArray { "Very Fast", "Fast", "Medium", "Slow", "Very Slow" }, 3));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "analyzer_tilt", 1 }, "Analyzer Tilt", juce::NormalisableRange<float> (-9.0f, 9.0f), 4.5f));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "analyzer_range", 1 }, "Analyzer Range", juce::StringArray { "60 dB", "90 dB", "120 dB" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "analyzer_reference_db", 1 }, "Analyzer Reference", juce::NormalisableRange<float> (-30.0f, 0.0f), -18.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "analyzer_freeze", 1 }, "Analyzer Freeze", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "analyzer_auto_range", 1 }, "Analyzer Auto Range", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "editor_width", 1 }, "Editor Width", juce::NormalisableRange<float> (800.0f, 2800.0f, 1.0f), 1400.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "editor_height", 1 }, "Editor Height", juce::NormalisableRange<float> (520.0f, 900.0f, 1.0f), 720.0f));

    return { params.begin(), params.end() };
}
