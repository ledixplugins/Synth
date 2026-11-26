#include "PluginEditor.h"
using namespace juce;

namespace {
const Colour kBg(0xff14181d), kPanel(0xff1f252c), kAmber(0xffe8890c), kText(0xffd9dde2);
struct Meta { const char* id; const char* label; };
const Meta kExtra[] = { {"velsens","TOQUE"},{"veldest","TOQUE AGE EM"},{"prio","PRIORIDADE"},{"atdest","PRESSAO"},{"hold","HOLD"},
  {"tun","TUNE"},{"tt","ESCALA"},{"kte","ERRO TECLA"},{"a440","A-440"},{"bu","PITCH +"},{"bd","PITCH -"},{"vcv","CURVA"},{"xp","TRANSP."},{"ch","CANAL"},{"xon","LIGAR"},{"xlvl","NIVEL"},{"xmode","DISPARO"},{"xthr","LIMIAR"},{"mm","MOD MIX"},{"ma","A=ENV FILTRO"},{"mb","B=LFO"},{"mx","A <-> B"},{"mo","MOD OSC"},{"mf","MOD FILTRO"},{"e1","OSC 1"},{"e2","OSC 2"},{"e3","OSC 3"},{"kts","PASSOS"},
  {"fxd","EFX1"},{"fxdr","DRIVE"},{"fxdp","PRESENCA"},{"fxdl","NIVEL"},{"fxcr","TAXA"},{"fxcb","BITS"},{"fxm","EFX2"},{"fxmr","VELOC."},{"fxmd","PROFUND."},{"fxmf","FEEDBACK"},{"fxmn","MANUAL"},{"fxmb","BALANCO"},{"fxrf","FREQ. ANEL"},{"fxrv","REVERB"},{"fxrt","TIPO"},{"fxrm","TEMPO"},{"fxrh","AGUDOS"},{"fxrl","NIVEL REV."},{"rg","ANEL OSC"},{"un","UNISSONO"},{"ud","ABERTURA"},
  {"fm","FORMA"},{"fk","ACIDO"},{"of","OSC2>FILTRO"},{"l2r","VELOC."},{"l2w","ONDA"},{"l2d","ATRASO"},{"l2k","REINICIA"},{"l2f","> FILTRO"},{"l2p","> PITCH"},{"l2m","> PWM"},{"pwe","ENV > PWM"},{"pe","ERRO ALEAT."},{"arpsw","SWING"},
  {"arpon","ARP"},{"arppat","PADRAO"},{"arpdiv","DIVISAO"},{"arpoct","OITAVAS"},{"arpgate","DURACAO"},{"arplatch","TRAVA"},{"arpsync","TEMPO HOST"},{"arpbpm","BPM"} };
String shortLabel(const String& id, const String& full) {
    for (auto& m : kExtra) if (id == m.id) return m.label;
    String s = full.replace("Osc 1 ", "").replace("Osc 2 ", "").replace("Osc 3 ", "").replace("Filtro ", "").replace("LFO ", "").replace("Volume ", "").replace("Delay ", "");
    return s.toUpperCase();
}
}

AuraEditor::AuraEditor(AuraProcessor& p) : AudioProcessorEditor(&p), proc(p) {
    addSection("OSCILADOR 1", { "wave", "pw1", "lvl", "oct" });
    addSection("OSCILADOR 2", { "w2", "pw2", "l2", "d2", "o2", "sync" });
    addSection("OSCILADOR 3", { "w3", "l3", "o3", "d3", "k3" });
    addSection("MIXER", { "sub", "sw", "nz", "nc", "ovl" });
    addSection("FILTRO", { "cut", "res", "fenv", "kt", "drv", "pol" });
    addSection("ENV. FILTRO", { "fa", "fd", "fs", "fr" });
    addSection("ENV. VOLUME", { "a", "d", "s", "r", "ec" });
    addSection("PITCH / GLIDE", { "glide", "pa", "pd", "leg" });
    addSection("LFO", { "lr", "lwv", "lp", "lf", "lw", "lsr" });
    addSection("DELAY", { "dt", "df", "dm" });
    addSection("SAIDA", { "vol", "velsens", "veldest", "prio", "atdest", "hold" });
    addSection("MOD MIX", { "mm", "ma", "mb", "mx", "mo", "mf" });
    addSection("OSC LIGADO / TECLADO", { "e1", "e2", "e3", "kts" });
    addSection("AFINACAO", { "tun", "tt", "kte", "a440" });
    addSection("CONTROLES", { "bu", "bd", "vcv", "xp", "ch" });
    addSection("ENTRADA EXTERNA", { "xon", "xlvl", "xmode", "xthr" });
    addSection("EFEITO 1", { "fxd", "fxdr", "fxdp", "fxcr", "fxcb", "fxdl" });
    addSection("EFEITO 2", { "fxm", "fxmr", "fxmd", "fxmf", "fxmn", "fxmb", "fxrf" });
    addSection("REVERB", { "fxrv", "fxrt", "fxrm", "fxrh", "fxrl" });
    addSection("OSC EXTRAS", { "rg", "un", "ud" });
    addSection("FILTRO AVANÇADO", { "fk", "fm", "of" });
    addSection("LFO 2", { "l2w", "l2r", "l2d", "l2k", "l2f", "l2p", "l2m" });
    addSection("MOD. EXTRA", { "pwe", "pe" });
    addSection("ARPEJADOR", { "arpon", "arppat", "arpdiv", "arpoct", "arpgate", "arpsw", "arpbpm", "arpsync", "arplatch" });

    for (auto* b : { &bSave, &bImp, &bExp, &bPrev, &bNext }) addAndMakeVisible(*b);
    addAndMakeVisible(presets);
    presets.onChange = [this] { int id = presets.getSelectedId(); if (id > 0 && id - 1 != proc.getCurrentProgram()) proc.setCurrentProgram(id - 1); };
    bPrev.onClick = [this] { int n = proc.getNumPrograms(); proc.setCurrentProgram((proc.getCurrentProgram() + n - 1) % n); };
    bNext.onClick = [this] { proc.setCurrentProgram((proc.getCurrentProgram() + 1) % proc.getNumPrograms()); };
    bSave.onClick = [this] {
        auto* w = new AlertWindow("Salvar preset", "Nome do preset:", MessageBoxIconType::NoIcon);
        w->addTextEditor("nome", "", ""); w->addButton("Salvar", 1); w->addButton("Cancelar", 0);
        w->enterModalState(true, ModalCallbackFunction::create([this, w](int r) { if (r == 1) proc.saveUserPreset(w->getTextEditorContents("nome")); delete w; }), false);
    };
    bImp.onClick = [this] {
        chooser = std::make_unique<FileChooser>("Importar presets (backup .json do AURA)", File(), "*.json");
        chooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this](const FileChooser& fc) {
            auto f = fc.getResult(); if (f.existsAsFile()) { int n = proc.importJson(f);
                AlertWindow::showMessageBoxAsync(MessageBoxIconType::InfoIcon, "Importar", String(n) + " preset(s) importado(s)."); } });
    };
    bExp.onClick = [this] {
        chooser = std::make_unique<FileChooser>("Exportar meus presets", File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("aura-presets.json"), "*.json");
        chooser->launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting, [this](const FileChooser& fc) {
            auto f = fc.getResult(); if (f != File()) proc.exportJson(f.withFileExtension("json")); });
    };
    proc.onPresetsChanged = [this] { MessageManager::callAsync([this] { refreshPresets(); }); };
    refreshPresets();
    startTimerHz(5);
    setResizable(true, true); setResizeLimits(700, 520, 1500, 1200); setSize(1040, 700);
}

AuraEditor::~AuraEditor() { proc.onPresetsChanged = nullptr; stopTimer(); }

void AuraEditor::addSection(const String& title, std::initializer_list<const char*> ids) {
    auto sec = std::make_unique<Section>(); sec->title = title;
    for (auto* id : ids) {
        auto* par = proc.apvts.getParameter(id); if (!par) continue;
        auto c = std::make_unique<Cell>(); c->id = id;
        c->lb = std::make_unique<Label>(); c->lb->setText(shortLabel(id, par->getName(40)), dontSendNotification);
        c->lb->setJustificationType(Justification::centred); c->lb->setFont(FontOptions(11.0f)); c->lb->setColour(Label::textColourId, kText);
        addAndMakeVisible(*c->lb);
        if (auto* ch = dynamic_cast<AudioParameterChoice*>(par)) {
            c->kind = 1; c->cb = std::make_unique<ComboBox>(); c->cb->addItemList(ch->choices, 1); addAndMakeVisible(*c->cb);
            c->ca = std::make_unique<ComboBoxParameterAttachment>(*ch, *c->cb);
        } else if (dynamic_cast<AudioParameterBool*>(par)) {
            c->kind = 2; c->tb = std::make_unique<ToggleButton>("ON"); addAndMakeVisible(*c->tb);
            c->ba = std::make_unique<ButtonParameterAttachment>(*par, *c->tb);
        } else {
            c->kind = 0; c->sl = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow);
            c->sl->setTextBoxStyle(Slider::TextBoxBelow, false, 74, 16); c->sl->setColour(Slider::rotarySliderFillColourId, kAmber);
            c->sl->setColour(Slider::textBoxTextColourId, kText); c->sl->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
            c->sl->setDoubleClickReturnValue(true, par->convertFrom0to1(par->getDefaultValue()));
            addAndMakeVisible(*c->sl);
            c->sa = std::make_unique<SliderParameterAttachment>(*par, *c->sl);
        }
        sec->cells.push_back(std::move(c));
    }
    sections.push_back(std::move(sec));
}

void AuraEditor::refreshPresets() {
    presets.clear(dontSendNotification);
    for (int i = 0; i < proc.getNumPrograms(); i++) presets.addItem(String(i + 1) + "  " + proc.getProgramName(i), i + 1);
    presets.setSelectedId(proc.getCurrentProgram() + 1, dontSendNotification);
    lastProgram = proc.getCurrentProgram(); lastCount = proc.getNumPrograms();
}
void AuraEditor::timerCallback() {
    if (proc.getNumPrograms() != lastCount) refreshPresets();
    else if (proc.getCurrentProgram() != lastProgram) { lastProgram = proc.getCurrentProgram(); presets.setSelectedId(lastProgram + 1, dontSendNotification); }
}

void AuraEditor::paint(Graphics& g) {
    g.fillAll(kBg);
    g.setColour(kAmber); g.setFont(FontOptions(18.0f, Font::bold)); g.drawText("AURA SYNTH", 12, 8, 220, 28, Justification::centredLeft);
    g.setColour(kText.withAlpha(0.5f)); g.setFont(FontOptions(10.0f)); g.drawText("(c) 2026 Thiago R. Galvao", 12, 30, 220, 14, Justification::centredLeft);
    for (auto& s : sections) {
        g.setColour(kPanel); g.fillRoundedRectangle(s->area.toFloat(), 6.0f);
        g.setColour(kAmber.withAlpha(0.8f)); g.setFont(FontOptions(11.0f, Font::bold)); g.drawText(s->title, s->area.withHeight(18).reduced(8, 0), Justification::centredLeft);
    }
}

void AuraEditor::resized() {
    auto top = getLocalBounds().reduced(8).removeFromTop(40);
    top.removeFromLeft(230);
    bExp.setBounds(top.removeFromRight(90).reduced(2)); bImp.setBounds(top.removeFromRight(90).reduced(2)); bSave.setBounds(top.removeFromRight(90).reduced(2));
    bPrev.setBounds(top.removeFromLeft(34).reduced(2)); bNext.setBounds(top.removeFromLeft(34).reduced(2)); presets.setBounds(top.reduced(2));
    const int cw = 78, ch = 92, pad = 6, hdr = 20, W = getWidth() - 16;
    int x = 8, y = 56, rowH = 0;
    for (auto& s : sections) {
        const int w = (int)s->cells.size() * cw + 2 * pad, h = hdr + ch + pad;
        if (x + w > 8 + W && x > 8) { x = 8; y += rowH + 8; rowH = 0; }
        s->area = { x, y, w, h };
        int cx = x + pad;
        for (auto& c : s->cells) {
            Rectangle<int> r(cx, y + hdr, cw, ch);
            c->lb->setBounds(r.removeFromTop(14));
            if (c->kind == 0) c->sl->setBounds(r);
            else if (c->kind == 1) c->cb->setBounds(r.withTrimmedTop(18).withHeight(24).reduced(2, 0));
            else c->tb->setBounds(r.withTrimmedTop(18).withHeight(26).reduced(8, 0));
            cx += cw;
        }
        x += w + 8; rowH = std::max(rowH, h);
    }
    repaint();
}
