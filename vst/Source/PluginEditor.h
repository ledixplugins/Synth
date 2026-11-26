// AURA Synth (plugin) - Copyright (c) 2026 Thiago R. Galvão. Todos os direitos reservados.
#pragma once
#include "PluginProcessor.h"

class AuraEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit AuraEditor(AuraProcessor&);
    ~AuraEditor() override;
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    struct Cell {
        juce::String id; int kind = 0;   // 0 knob, 1 combo, 2 toggle
        std::unique_ptr<juce::Slider> sl; std::unique_ptr<juce::ComboBox> cb; std::unique_ptr<juce::ToggleButton> tb; std::unique_ptr<juce::Label> lb;
        std::unique_ptr<juce::SliderParameterAttachment> sa; std::unique_ptr<juce::ComboBoxParameterAttachment> ca; std::unique_ptr<juce::ButtonParameterAttachment> ba;
    };
    struct Section { juce::String title; std::vector<std::unique_ptr<Cell>> cells; juce::Rectangle<int> area; };
    void addSection(const juce::String& title, std::initializer_list<const char*> ids);
    void refreshPresets();
    void timerCallback() override;
    AuraProcessor& proc;
    std::vector<std::unique_ptr<Section>> sections;
    juce::ComboBox presets; juce::TextButton bSave{ "Salvar..." }, bImp{ "Importar..." }, bExp{ "Exportar..." }, bPrev{ "<" }, bNext{ ">" };
    std::unique_ptr<juce::FileChooser> chooser;
    int lastProgram = -1, lastCount = -1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuraEditor)
};
