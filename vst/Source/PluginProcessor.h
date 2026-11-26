// AURA Synth (plugin) - Copyright (c) 2026 Thiago R. Galvão. Todos os direitos reservados. ID: AURA-SB-TRG-2026
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "AuraEngine.h"
#include "Fx.h"
#include "Arp.h"
#include "Presets.h"
#include "ParamDefs.h"

struct UserPreset { juce::String name; float v[aura::NP]; };

class AuraProcessor : public juce::AudioProcessor {
public:
    AuraProcessor();
    ~AuraProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override { auto o = l.getMainOutputChannelSet(); auto i = l.getMainInputChannelSet(); return (o == juce::AudioChannelSet::stereo() || o == juce::AudioChannelSet::mono()) && (i.isDisabled() || i == juce::AudioChannelSet::stereo() || i == juce::AudioChannelSet::mono()); }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "AURA Synth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return aura::kNumFactory + (int)user.size(); }
    int getCurrentProgram() override { return program; }
    void setCurrentProgram(int i) override;
    void changeProgramName(int, const juce::String&) override {}
    const juce::String getProgramName(int i) override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // presets do usuário
    void saveUserPreset(const juce::String& name);
    int importJson(const juce::File& f);
    bool exportJson(const juce::File& f) const;
    int userCount() const { return (int)user.size(); }
    std::function<void()> onPresetsChanged;

    juce::AudioProcessorValueTreeState apvts;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    void applyValues(const float* v);
    void applyFx(const float* f);
    void applyArp(const float* a);
    void setParamNorm(const char* id, float real);
    aura::Engine engine; aura::Arp arp;
    std::atomic<float>* raw[aura::NP] = {};
    std::atomic<float>* pArpSw = nullptr;
    std::atomic<float> *pTun, *pKte, *pTt, *pBu, *pBd, *pVcv, *pXp, *pCh, *pA440, *pXon, *pXlvl, *pXmode, *pXthr;
    static constexpr int kFxN = 18; std::atomic<float>* pFx[kFxN] = {}; aura::Fx fx; std::vector<float> fxTmp, fxR;
    std::vector<float> extIn; double tonePh = 0;
    std::atomic<float> *pVelSens, *pVelDest, *pPrio, *pAtDest, *pHold, *pArpOn, *pArpPat, *pArpDiv, *pArpOct, *pArpGate, *pArpLatch, *pArpSync, *pArpBpm;
    std::vector<UserPreset> user;
    int program = 0; bool pedal = false, lastArpOn = false, arpDoPreset = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuraProcessor)
};
