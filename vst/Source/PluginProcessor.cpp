// AURA Synth (plugin) - Copyright (c) 2026 Thiago R. Galvão. Todos os direitos reservados. ID: AURA-SB-TRG-2026
#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;
using aura::kDefs; using aura::Kind;

namespace {
// valores padrão (iguais ao DEF da versão web), para presets importados que não trazem todos os campos
const float kDefault[aura::NP] = {-1,2,0.5f,0.75f,1,0.5f,0,0,0,0,0,0,0,0.5f,0,0,0,0,4,0,0,0.15f,5.5f,0,0,0,0,0.002f,0.3f,0,0.2f,0.004f,0.3f,1,0.1f,1,1.3f,0,1,0,0,1,0,0,0,0,0.3f,0.35f,0, 0,0,1,0.5f,1,1,1,1,1,0, 0,0,0.4f, 0,0,4,0,0,0,0,0,0,0,0,0};
int waveFromVar(const var& v, int d) {
    if (v.isString()) { auto s = v.toString(); if (s == "triangle") return 0; if (s == "sawtooth") return 1; if (s == "square") return 2; if (s == "rsaw") return 3; if (s == "tsaw") return 4; }
    return v.isVoid() ? d : (int)v;
}
}

AudioProcessorValueTreeState::ParameterLayout AuraProcessor::makeLayout() {
    AudioProcessorValueTreeState::ParameterLayout L;
    for (int i = 0; i < aura::NP; i++) {
        const aura::Def& d = kDefs[i]; ParameterID pid(d.id, 1);
        switch (d.k) {
        case aura::F: { NormalisableRange<float> r(d.mn, d.mx); r.skew = 1.0f / d.ex;
                  AudioParameterFloatAttributes at;
                  if (std::string(d.id) == "cut") at = at.withStringFromValueFunction([](float v, int) { return String((int)std::round(30 * std::pow(600.0, v))) + " Hz"; });
                  L.add(std::make_unique<AudioParameterFloat>(pid, d.name, r, aura::kFactory[0].v[i], at)); break; }
        case aura::I: L.add(std::make_unique<AudioParameterInt>(pid, d.name, (int)d.mn, (int)d.mx, (int)aura::kFactory[0].v[i])); break;
        case aura::B: L.add(std::make_unique<AudioParameterBool>(pid, d.name, aura::kFactory[0].v[i] > 0.5f)); break;
        case aura::W: L.add(std::make_unique<AudioParameterChoice>(pid, d.name, StringArray{ "Triangular", "Serra", "Quadrada", "Serra reversa", "Tri/Serra" }, (int)aura::kFactory[0].v[i])); break;
        case aura::LW: L.add(std::make_unique<AudioParameterChoice>(pid, d.name, StringArray{ "Triangular", "Quadrada" }, (int)aura::kFactory[0].v[i])); break;
        }
    }
    L.add(std::make_unique<AudioParameterFloat>(ParameterID("velsens", 1), "Sensibilidade ao toque", NormalisableRange<float>(0, 1), 0.75f));
    L.add(std::make_unique<AudioParameterChoice>(ParameterID("veldest", 1), "Toque age em", StringArray{ "Volume e brilho", "So volume", "So brilho" }, 0));
    L.add(std::make_unique<AudioParameterChoice>(ParameterID("prio", 1), "Prioridade de nota", StringArray{ "Ultima nota", "Mais grave", "Mais aguda" }, 0));
    L.add(std::make_unique<AudioParameterChoice>(ParameterID("atdest", 1), "Pressao de canal", StringArray{ "Desligada", "Vibrato", "Filtro", "Volume" }, 0));
    L.add(std::make_unique<AudioParameterBool>(ParameterID("hold", 1), "Hold", false));
    L.add(std::make_unique<AudioParameterBool>(ParameterID("arpon", 1), "Arpejador", false));
    L.add(std::make_unique<AudioParameterChoice>(ParameterID("arppat", 1), "Arp Padrao", StringArray{ "Sobe", "Desce", "Sobe e desce", "Aleatorio", "Ordem" }, 0));
    L.add(std::make_unique<AudioParameterChoice>(ParameterID("arpdiv", 1), "Arp Divisao", StringArray{ "1/4", "1/8", "1/8T", "1/16" }, 1));
    L.add(std::make_unique<AudioParameterInt>(ParameterID("arpoct", 1), "Arp Oitavas", 1, 3, 1));
    L.add(std::make_unique<AudioParameterFloat>(ParameterID("arpgate", 1), "Arp Duracao", NormalisableRange<float>(0.1f, 0.95f), 0.6f));
    L.add(std::make_unique<AudioParameterBool>(ParameterID("arplatch", 1), "Arp Trava", false));
    L.add(std::make_unique<AudioParameterBool>(ParameterID("arpsync", 1), "Arp Tempo do host", true));
    // efeitos (inspirados no JD-Xi)
    L.add(std::make_unique<AudioParameterChoice>(ParameterID("fxd", 1), "EFX1 Tipo", StringArray{ "Desligado", "Distorcao", "Fuzz", "Bitcrusher" }, 0));
    L.add(std::make_unique<AudioParameterChoice>(ParameterID("fxm", 1), "EFX2 Tipo", StringArray{ "Desligado", "Chorus", "Flanger", "Phaser", "Anel" }, 0));
    L.add(std::make_unique<AudioParameterBool>(ParameterID("fxrv", 1), "Reverb", false));
    L.add(std::make_unique<AudioParameterChoice>(ParameterID("fxrt", 1), "Reverb Tipo", StringArray{ "Sala 1", "Sala 2", "Palco 1", "Palco 2", "Catedral 1", "Catedral 2" }, 1));
    { static const struct { const char* id; const char* nm; float def; } F[] = { {"fxdr","Drive",0.4f},{"fxdp","Presenca",0.6f},{"fxdl","EFX1 Nivel",0.7f},{"fxcr","Taxa",0.4f},{"fxcb","Bits",0.5f},
        {"fxmr","Velocidade",0.3f},{"fxmd","Profundidade",0.5f},{"fxmf","Feedback/Res.",0.2f},{"fxmn","Manual",0.5f},{"fxmb","Balanco",0.5f},{"fxrf","Freq. Anel",0.35f},
        {"fxrm","Reverb Tempo",0.5f},{"fxrh","Reverb Agudos",0.7f},{"fxrl","Reverb Nivel",0.3f} };
      for (auto& f : F) L.add(std::make_unique<AudioParameterFloat>(ParameterID(f.id, 1), f.nm, NormalisableRange<float>(0, 1), f.def)); }
    L.add(std::make_unique<AudioParameterFloat>(ParameterID("tun", 1), "Tune (cent)", NormalisableRange<float>(-100, 100, 1), 0));
    L.add(std::make_unique<AudioParameterBool>(ParameterID("kte", 1), "Erro de tecla", false));
    L.add(std::make_unique<AudioParameterChoice>(ParameterID("tt", 1), "Escala", StringArray{ "Igual", "Pitagorica", "Carlos", "Partch 43" }, 0));
    L.add(std::make_unique<AudioParameterInt>(ParameterID("bu", 1), "Pitch para cima (st)", 0, 12, 2));
    L.add(std::make_unique<AudioParameterInt>(ParameterID("bd", 1), "Pitch para baixo (st)", 0, 12, 12));
    L.add(std::make_unique<AudioParameterChoice>(ParameterID("vcv", 1), "Curva de toque", StringArray{ "Suave", "Media", "Dura" }, 1));
    L.add(std::make_unique<AudioParameterChoice>(ParameterID("xp", 1), "Transposicao MIDI", StringArray{ "-12", "0", "+12" }, 1));
    L.add(std::make_unique<AudioParameterInt>(ParameterID("ch", 1), "Canal MIDI (0 = todos)", 0, 16, 0));
    L.add(std::make_unique<AudioParameterBool>(ParameterID("a440", 1), "Tom A-440", false));
    L.add(std::make_unique<AudioParameterBool>(ParameterID("xon", 1), "Entrada externa", false));
    L.add(std::make_unique<AudioParameterFloat>(ParameterID("xlvl", 1), "Entrada nivel", NormalisableRange<float>(0, 1), 0.5f));
    L.add(std::make_unique<AudioParameterChoice>(ParameterID("xmode", 1), "Entrada disparo", StringArray{ "Pelas teclas", "Pelo sinal" }, 0));
    L.add(std::make_unique<AudioParameterFloat>(ParameterID("xthr", 1), "Entrada limiar", NormalisableRange<float>(0.01f, 0.6f), 0.15f));
    L.add(std::make_unique<AudioParameterFloat>(ParameterID("arpsw", 1), "Arp Swing", NormalisableRange<float>(0, 1), 0));
    L.add(std::make_unique<AudioParameterFloat>(ParameterID("arpbpm", 1), "Arp Andamento", NormalisableRange<float>(40, 240, 1), 100));
    return L;
}

AuraProcessor::AuraProcessor()
    : AudioProcessor(BusesProperties().withInput("Entrada", AudioChannelSet::stereo(), true).withOutput("Output", AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "AURA", makeLayout()) {
    for (int i = 0; i < aura::NP; i++) raw[i] = apvts.getRawParameterValue(kDefs[i].id);
    auto g = [this](const char* id) { return apvts.getRawParameterValue(id); };
    pVelSens = g("velsens"); pVelDest = g("veldest"); pPrio = g("prio"); pAtDest = g("atdest"); pHold = g("hold");
    pArpOn = g("arpon"); pArpPat = g("arppat"); pArpDiv = g("arpdiv"); pArpOct = g("arpoct"); pArpGate = g("arpgate"); pArpLatch = g("arplatch"); pArpSync = g("arpsync"); pArpBpm = g("arpbpm"); pArpSw = g("arpsw");
    { static const char* ids[kFxN] = { "fxd","fxm","fxrv","fxrt","fxdr","fxdp","fxdl","fxcr","fxcb","fxmr","fxmd","fxmf","fxmn","fxmb","fxrf","fxrm","fxrh","fxrl" }; for (int i = 0; i < kFxN; i++) pFx[i] = g(ids[i]); }
    pTun = g("tun"); pKte = g("kte"); pTt = g("tt"); pBu = g("bu"); pBd = g("bd"); pVcv = g("vcv"); pXp = g("xp"); pCh = g("ch"); pA440 = g("a440"); pXon = g("xon"); pXlvl = g("xlvl"); pXmode = g("xmode"); pXthr = g("xthr");
    engine.prepare(44100); fx.prepare(44100);
}

void AuraProcessor::prepareToPlay(double sr, int) {
    for (int i = 0; i < aura::NP; i++) engine.p[i] = raw[i]->load();
    engine.prepare(sr); fx.prepare(sr); arp.clear();
}

const String AuraProcessor::getProgramName(int i) {
    if (i >= 0 && i < aura::kNumFactory) return String(aura::kFactory[i].name);
    i -= aura::kNumFactory; if (i >= 0 && i < (int)user.size()) return user[(size_t)i].name + " (meu)";
    return {};
}

void AuraProcessor::applyValues(const float* v) {
    for (int i = 0; i < aura::NP; i++)
        if (auto* p = apvts.getParameter(kDefs[i].id)) { p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1(v[i])); p->endChangeGesture(); }
}

void AuraProcessor::setCurrentProgram(int idx) {
    if (idx < 0 || idx >= getNumPrograms()) return;
    program = idx;
    if (idx < aura::kNumFactory) {
        applyValues(aura::kFactory[idx].v);
        if (aura::kFactory[idx].temFx) applyFx(aura::kFactory[idx].fx);
        if (aura::kFactory[idx].temArp) { applyArp(aura::kFactory[idx].arp); arpDoPreset = true; }
        else if (arpDoPreset) { setParamNorm("arpon", 0.f); arpDoPreset = false; }
    } else {
        applyValues(user[(size_t)(idx - aura::kNumFactory)].v);
        if (arpDoPreset) { setParamNorm("arpon", 0.f); arpDoPreset = false; }
    }
}

void AuraProcessor::setParamNorm(const char* id, float real) {
    if (auto* p = apvts.getParameter(id)) { p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1(real)); p->endChangeGesture(); }
}

void AuraProcessor::applyArp(const float* a) {
    for (int i = 0; i < aura::kNumArp; i++) setParamNorm(aura::kArpIds[i], a[i]);
}

void AuraProcessor::applyFx(const float* f) {
    for (int i = 0; i < aura::kNumFx; i++)
        if (auto* p = apvts.getParameter(aura::kFxIds[i])) { p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1(f[i])); p->endChangeGesture(); }
}

void AuraProcessor::saveUserPreset(const String& nameIn) {
    String name = nameIn.trim().toUpperCase().substring(0, 18); if (name.isEmpty()) name = "SEM NOME";
    UserPreset u; u.name = name;
    for (int i = 0; i < aura::NP; i++) u.v[i] = raw[i]->load();
    int at = -1; for (size_t k = 0; k < user.size(); k++) if (user[k].name == name) at = (int)k;
    if (at >= 0) user[(size_t)at] = u; else { user.push_back(u); at = (int)user.size() - 1; }
    program = aura::kNumFactory + at;
    updateHostDisplay(); if (onPresetsChanged) onPresetsChanged();
}

int AuraProcessor::importJson(const File& f) {
    auto j = JSON::parse(f); auto* arr = j.getProperty("presets", var()).getArray(); int n = 0;
    if (!arr) return 0;
    for (auto& e : *arr) {
        if (!e.isObject() || !e.hasProperty("name")) continue;
        UserPreset u; u.name = e["name"].toString().trim().toUpperCase().substring(0, 18); if (u.name.isEmpty()) continue;
        for (int i = 0; i < aura::NP; i++) {
            const auto id = kDefs[i].id; float v = kDefault[i];
            if (e.hasProperty(id)) {
                if (kDefs[i].k == aura::W) v = (float)waveFromVar(e[id], (int)v);
                else if (kDefs[i].k == aura::LW) v = e[id].toString() == "square" ? 1.0f : 0.0f;
                else v = (float)(double)e[id];
            }
            v = jlimit(kDefs[i].mn, kDefs[i].mx, v); u.v[i] = v;
        }
        if (!e.hasProperty("fa")) { u.v[aura::fa] = u.v[aura::a]; u.v[aura::fd] = u.v[aura::d]; u.v[aura::fs] = u.v[aura::s]; u.v[aura::fr] = u.v[aura::r]; }
        int at = -1; for (size_t k = 0; k < user.size(); k++) if (user[k].name == u.name) at = (int)k;
        if (at >= 0) user[(size_t)at] = u; else user.push_back(u);
        n++;
    }
    if (n) { updateHostDisplay(); if (onPresetsChanged) onPresetsChanged(); }
    return n;
}

bool AuraProcessor::exportJson(const File& f) const {
    Array<var> list;
    static const char* wn[5] = { "triangle", "sawtooth", "square", "rsaw", "tsaw" };
    for (auto& u : user) {
        auto* o = new DynamicObject(); o->setProperty("name", u.name);
        for (int i = 0; i < aura::NP; i++) {
            const auto id = kDefs[i].id;
            if (kDefs[i].k == aura::W) o->setProperty(id, String(wn[jlimit(0, 4, (int)u.v[i])]));
            else if (kDefs[i].k == aura::LW) o->setProperty(id, String(u.v[i] > 0.5f ? "square" : "triangle"));
            else o->setProperty(id, (double)u.v[i]);
        }
        list.add(var(o));
    }
    auto* root = new DynamicObject(); root->setProperty("aura", 1); root->setProperty("origem", "AURA-SB-TRG-2026 (c) 2026 Thiago R. Galvao"); root->setProperty("presets", list);
    return f.replaceWithText(JSON::toString(var(root)));
}

void AuraProcessor::processBlock(AudioBuffer<float>& buf, MidiBuffer& midi) {
    ScopedNoDenormals nd;
    const int n = buf.getNumSamples(), nch = buf.getNumChannels();
    for (int i = 0; i < aura::NP; i++) engine.p[i] = raw[i]->load();
    engine.velSens = pVelSens->load(); engine.velDest = (int)pVelDest->load(); engine.prio = (int)pPrio->load(); engine.atDest = (int)pAtDest->load();
    if (nch < 1) return;
    engine.tun = pTun->load(); engine.kte = (int)pKte->load(); engine.tt = (int)pTt->load(); engine.bu = pBu->load(); engine.bd = pBd->load(); engine.vcv = (int)pVcv->load();
    engine.xlvl = pXon->load() > 0.5f ? pXlvl->load() : 0.0f; engine.xmode = (int)pXmode->load(); engine.xthr = pXthr->load();
    const int xpose = ((int)pXp->load() - 1) * 12, chan = (int)pCh->load();
    const bool tone = pA440->load() > 0.5f;
    // entrada externa em mono (soma dos canais de entrada)
    const float* inPtr = nullptr;
    if (getTotalNumInputChannels() > 0 && engine.xlvl > 0) {
        extIn.assign((size_t)n, 0.0f);
        for (int c = 0; c < getTotalNumInputChannels() && c < nch; c++) { auto* r = buf.getReadPointer(c); for (int i = 0; i < n; i++) extIn[(size_t)i] += r[i]; }
        const float g = 1.0f / (float)std::max(1, std::min(getTotalNumInputChannels(), nch));
        for (auto& v : extIn) v *= g;
        inPtr = extIn.data();
    }
    // arpejador
    const bool arpOn = pArpOn->load() > 0.5f;
    arp.pat = (int)pArpPat->load(); arp.div = (int)pArpDiv->load(); arp.oct = (int)pArpOct->load(); arp.gate = pArpGate->load(); arp.latch = pArpLatch->load() > 0.5f;
    double bpm = pArpBpm->load();
    if (pArpSync->load() > 0.5f) if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto b = pos->getBpm()) bpm = *b;
    arp.bpm = bpm; arp.swing = pArpSw->load();
    auto offF = [this](int note) { engine.realOff(note); };
    auto onF = [this](int note, int vel) { engine.coreOn(note, vel); };
    if (arpOn != lastArpOn) {
        lastArpOn = arpOn;
        if (arpOn) { arp.clear(); engine.allOff(); } else { arp.silence(offF); arp.clear(); engine.allOff(); }
    }
    engine.setSustain(pedal || pHold->load() > 0.5f);
    float* out = buf.getWritePointer(0);
    int pos = 0;
    const double sr = engine.sr;
    auto render = [&](int upto) {
        while (pos < upto) {
            int len = upto - pos;
            if (arpOn) { double e = arp.samplesToEvent(); if (e < len) len = std::max(1, (int)std::ceil(e)); }
            engine.process(out + pos, len, inPtr ? inPtr + pos : nullptr); pos += len;
            if (arpOn) arp.advance(len, sr, onF, offF);
        }
    };
    for (const auto meta : midi) {
        const auto m = meta.getMessage();
        if (chan > 0 && m.getChannel() != 0 && m.getChannel() != chan) continue;
        render(meta.samplePosition);
        const int nt = m.isNoteOnOrOff() ? jlimit(0, 127, m.getNoteNumber() + xpose) : 0;
        if (m.isNoteOn()) { if (arpOn) arp.press(nt, m.getVelocity() ? m.getVelocity() : 100); else engine.noteOn(nt, m.getVelocity()); }
        else if (m.isNoteOff()) { if (arpOn) arp.release(nt, offF); else engine.noteOff(nt); }
        else if (m.isPitchWheel()) engine.bend = jlimit(-1.0f, 1.0f, (m.getPitchWheelValue() - 8192) / 8192.0f);
        else if (m.isChannelPressure()) engine.atVal = m.getChannelPressureValue() / 127.0f;
        else if (m.isController()) {
            const int c = m.getControllerNumber(), v = m.getControllerValue();
            if (c == 1) engine.mod = v / 127.0f;
            else if (c == 64) { pedal = v >= 64; engine.setSustain(pedal || pHold->load() > 0.5f); }
            else if (c == 123 || c == 120) { arp.silence(offF); arp.clear(); engine.allOff(); }
        } else if (m.isAllNotesOff() || m.isAllSoundOff()) { arp.silence(offF); arp.clear(); engine.allOff(); }
    }
    render(n);
    if (tone) { for (int i = 0; i < n; i++) { out[i] += 0.25f * (float)std::sin(tonePh); tonePh += 6.283185307179586 * 440.0 / engine.sr; if (tonePh > 6.283185307179586) tonePh -= 6.283185307179586; } }
    {   // efeitos: mono -> estéreo
        aura::FxParams q; q.d = (int)pFx[0]->load(); q.m = (int)pFx[1]->load(); q.rv = pFx[2]->load() > 0.5f ? 1 : 0; q.rt = (int)pFx[3]->load();
        q.dr = pFx[4]->load(); q.dp = pFx[5]->load(); q.dl = pFx[6]->load(); q.cr = pFx[7]->load(); q.cb = pFx[8]->load(); q.mr = pFx[9]->load(); q.md = pFx[10]->load(); q.mf = pFx[11]->load();
        q.mn = pFx[12]->load(); q.mb = pFx[13]->load(); q.rf = pFx[14]->load(); q.rm = pFx[15]->load(); q.rh = pFx[16]->load(); q.rl = pFx[17]->load();
        if (q.d || q.m || q.rv) {
            fxTmp.assign(out, out + n); fxR.assign((size_t)n, 0.f);
            fx.process(q, fxTmp.data(), out, fxR.data(), n);
            auto sc = [](float x) { const float a = std::fabs(x); return a < 0.9f ? x : std::copysign(0.9f + 0.1f * std::tanh((a - 0.9f) / 0.1f), x); };
            for (int i = 0; i < n; i++) { out[i] = sc(out[i]); fxR[(size_t)i] = sc(fxR[(size_t)i]); }
            if (nch >= 2) buf.copyFrom(1, 0, fxR.data(), n);
            for (int c = 2; c < nch; c++) buf.copyFrom(c, 0, buf, 0, 0, n);
            return;
        }
    }
    for (int c = 1; c < nch; c++) buf.copyFrom(c, 0, buf, 0, 0, n);
}

AudioProcessorEditor* AuraProcessor::createEditor() { return new AuraEditor(*this); }

void AuraProcessor::getStateInformation(MemoryBlock& dest) {
    auto st = apvts.copyState(); st.setProperty("program", program, nullptr);
    ValueTree us("USERPRESETS");
    for (auto& u : user) { ValueTree p("P"); p.setProperty("name", u.name, nullptr); String s; for (int i = 0; i < aura::NP; i++) s << u.v[i] << ","; p.setProperty("v", s, nullptr); us.appendChild(p, nullptr); }
    st.addChild(us, -1, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary(*xml, dest);
}
void AuraProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size)) {
        auto st = ValueTree::fromXml(*xml);
        if (!st.isValid()) return;
        user.clear();
        auto us = st.getChildWithName("USERPRESETS");
        for (auto p : us) { UserPreset u; u.name = p["name"].toString(); auto t = StringArray::fromTokens(p["v"].toString(), ",", ""); for (int i = 0; i < aura::NP; i++) u.v[i] = i < t.size() ? t[i].getFloatValue() : kDefault[i]; user.push_back(u); }
        if (us.isValid()) st.removeChild(us, nullptr);
        program = (int)st.getProperty("program", 0); st.removeProperty("program", nullptr);
        apvts.replaceState(st);
        if (onPresetsChanged) onPresetsChanged();
    }
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AuraProcessor(); }
