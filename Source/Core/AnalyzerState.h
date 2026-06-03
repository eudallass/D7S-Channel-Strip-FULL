#pragma once

#include <array>
#include <atomic>

struct AnalyzerState
{
    static constexpr int numBins = 96;

    std::array<std::atomic<float>, numBins> preDb {};
    std::array<std::atomic<float>, numBins> postDb {};
    std::array<std::atomic<float>, numBins> prePeakDb {};
    std::array<std::atomic<float>, numBins> postPeakDb {};

    std::atomic<bool> preEnabled { true };
    std::atomic<bool> postEnabled { true };
    std::atomic<bool> freeze { false };
    std::atomic<bool> autoRange { false };
    std::atomic<int> resolutionIndex { 1 };
    std::atomic<int> speedIndex { 3 };
    std::atomic<int> rangeIndex { 1 };
    std::atomic<float> tiltDbPerOct { 4.5f };
    std::atomic<float> referenceDb { -18.0f };

    void reset()
    {
        for (auto& v : preDb) v.store (-96.0f);
        for (auto& v : postDb) v.store (-96.0f);
        for (auto& v : prePeakDb) v.store (-96.0f);
        for (auto& v : postPeakDb) v.store (-96.0f);
    }
};
