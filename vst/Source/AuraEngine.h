// AURA Synth - motor de som em C++ puro (porte do AudioWorklet da versão web)
// Copyright (c) 2026 Thiago R. Galvão. Todos os direitos reservados. ID: AURA-SB-TRG-2026
#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace aura {

enum P { oct, wave, pw1, lvl, w2, pw2, l2, d2, o2, sync, sub, sw, nz, cut, res, fenv, kt, drv, pol, glide, pa, pd, lr, lwv, lp, lf, lw,
         fa, fd, fs, fr, a, d, s, r, leg, vol, l3, w3, o3, d3, k3, nc, ovl, lsr, ec, dt, df, dm,
         mm, ma, mb, mx, mo, mf, e1, e2, e3, kts, rg, un, ud,
         fm, fk, l2r, l2w, l2d, l2k, l2f, l2p, l2m, of, pwe, pe, NP };

static const double TWO_PI = 6.283185307179586;

struct Engine {
    // ----- parâmetros (mesma ordem de Presets.h) e controles globais
    float p[NP] = {};
    float velSens = 0.75f;    // 0 a 1
    int   velDest = 0;        // 0 volume+brilho, 1 só volume, 2 só brilho
    int   prio = 0;           // 0 última, 1 grave, 2 aguda
    int   atDest = 0;         // 0 off, 1 vibrato, 2 filtro, 3 volume
    // ajustes globais (iguais aos do app web)
    float tun = 0; int kte = 0, tt = 0; float bu = 2, bd = 12; int vcv = 1;
    float xlvl = 0, xthr = 0.15f; int xmode = 0;   // entrada externa
    double o2sv = 0, pu[3] = { 0, 0, 0 }, q2p = 0, q2s = 0, q2t = 1e9, prC = 0;
    void newNote() { q2t = 0; if (p[l2k] > 0.5f) q2p = 0; prC = p[pe] > 0 ? (rnd() * 2 - 1) * p[pe] * 100.0 : 0.0; }
    double mvm = 0, nn0 = 0, nn1 = 0, nn2 = 0, rdN = 0, fe = 0, feStep = 0, feStep2 = 0, xe = 0; int xtr = 0, xh = 0; int xNote = -1;
    double sr = 44100;

    // ----- ruído
    uint32_t rs = 22222u;
    inline double rnd() { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return (rs >> 8) * (1.0 / 8388608.0); }   // [0,1)

    // ----- voz
    double p1 = 0, p2 = 0, ps = 0, p3 = 0, y1 = 0, y2 = 0, y3 = 0, y4 = 0, b0 = 0, b1 = 0, b2 = 0, hx = 0, hy = 0, mv = 0, shT = 0, shV = 0, ha = 0;
    double ml1 = .75, ml2 = 0, mls = 0, mnz = 0, ml3 = 0, movl = 0, mres = 0, mdrv = 0; bool mfirst = true;
    double smRes = 0.35;

    // ----- envelopes
    enum { IDLE, ATT, DEC, REL };
    int ast = IDLE; double aval = 0, astep = 0, atc = 0, atarget = 0, alin = 0;
    int cst = IDLE; double cval = 0, cstep = 0, ctarget = 0, cmul = 1, ccoef = 0; bool cexp = false;
    double cutNow = 1000;
    double freq = 55, freqT = 55, glideC = 0;
    double pitchC = 0, pitchK = 0;
    double velGain = 0.883, velGainT = 0.883, velBright = 1;
    double lfoPh = 0;
    double masterG = 1;
    bool gate = false;
    int lastNote = 36;
    std::vector<int> stack;
    std::vector<int> held; bool sustainOn = false;
    float bend = 0, mod = 0, atVal = 0;

    // ----- compressor
    double lt, kt_, kc_k, yk, kdb, slp, mk, ce = 0, cg = 1, cgt = 1; int cn = 0; double cer, cca, ccr;
    // ----- limitador / delay
    std::vector<float> dline; int dw = 0;
    double dlb0, dlb1, dlb2, dla1, dla2, dz1 = 0, dz2 = 0;
    double dtS = 0.3, dfS = 0.35, dmS = 0, preOut = 0;

    static double dbf(double x) { return 20 * std::log10(x); }
    double kcf(double x, double k) const { return lt + (1 - std::exp(-k * (x - lt))) / k; }
    double slopeAt(double x, double k) const { if (x < lt) return 1; double x2 = x * 1.001; return (dbf(kcf(x2, k)) - dbf(kcf(x, k))) / (dbf(x2) - dbf(x)); }
    double sat(double x) const { return x < lt ? x : (x < kt_ ? kcf(x, kc_k) : std::pow(10, (yk + slp * (dbf(x) - kdb)) / 20)); }

    void prepare(double sampleRate) {
        sr = sampleRate;
        ha = std::exp(-TWO_PI * 25 / sr);
        const double th = -14, kn = 10, ra = 5;
        lt = std::pow(10, th / 20); kt_ = std::pow(10, (th + kn) / 20); slp = 1 / ra;
        double lo = 0.1, hi = 10000, k = 5;
        for (int i = 0; i < 15; i++) { if (slopeAt(kt_, k) < slp) hi = k; else lo = k; k = std::sqrt(lo * hi); }
        kc_k = k; yk = dbf(kcf(kt_, k)); kdb = th + kn;
        mk = std::pow(1 / sat(1), 0.6);
        cer = std::exp(-1 / (0.03 * sr)); cca = 1 - std::exp(-4 / (0.00008 * sr)); ccr = 1 - std::exp(-4 / (0.05 * sr));
        dline.assign((size_t)(sr * 1.6) + 8, 0.f); dw = 0;
        // passa-baixas 4200 Hz Butterworth (igual ao biquad do navegador, Q 0,707 aprox.)
        double w0 = TWO_PI * 4200 / sr, al = std::sin(w0) / (2 * 0.7071), cs = std::cos(w0), a0 = 1 + al;
        dlb0 = (1 - cs) / 2 / a0; dlb1 = (1 - cs) / a0; dlb2 = dlb0; dla1 = -2 * cs / a0; dla2 = (1 - al) / a0;
        reset();
    }
    void reset() {
        p1 = p2 = ps = p3 = y1 = y2 = y3 = y4 = b0 = b1 = b2 = hx = hy = mv = 0; mfirst = true;
        ast = cst = IDLE; aval = 0; gate = false; stack.clear(); held.clear(); sustainOn = false;
        ce = 0; cg = 1; cgt = 1; cn = 0; dz1 = dz2 = 0; std::fill(dline.begin(), dline.end(), 0.f);
        cutNow = baseCut(); freq = freqT = noteFreq(lastNote); pitchC = 0;
    }

    // ----- utilidades
    static double mtof(double n) { return 440 * std::pow(2, (n - 69) / 12); }
    static double keyErr(int n) { uint32_t h = (uint32_t)(n + 1) * 2654435761u; return ((h % 2001u) / 1000.0 - 1) * 10; }
    double tuneHz(int n) const {
        static const double PYT[12] = { 1, 256.0/243, 9.0/8, 32.0/27, 81.0/64, 4.0/3, 729.0/512, 3.0/2, 128.0/81, 27.0/16, 16.0/9, 243.0/128 };
        static const double CAR[12] = { 16, 17, 18, 19, 20, 21, 22, 24, 26, 27, 28, 30 };
        static const double PAR[43] = { 1, 81.0/80, 33.0/32, 21.0/20, 16.0/15, 12.0/11, 11.0/10, 10.0/9, 9.0/8, 8.0/7, 7.0/6, 32.0/27, 6.0/5, 11.0/9, 5.0/4, 14.0/11, 9.0/7, 21.0/16, 4.0/3, 27.0/20, 11.0/8, 7.0/5, 10.0/7, 16.0/11, 40.0/27, 3.0/2, 32.0/21, 14.0/9, 11.0/7, 8.0/5, 18.0/11, 5.0/3, 27.0/16, 12.0/7, 7.0/4, 16.0/9, 9.0/5, 20.0/11, 11.0/6, 15.0/8, 40.0/21, 64.0/33, 160.0/81 };
        double f;
        if (tt == 0) f = mtof(n);
        else if (tt <= 2) { int pc = ((n % 12) + 12) % 12, base = n - pc; f = mtof(base) * (tt == 1 ? PYT[pc] : CAR[pc] / 16.0); }
        else { int d = n - 36, L = 43, deg = ((d % L) + L) % L, oc = (int)std::floor(d / (double)L); f = 65.40639 * std::pow(2.0, oc) * PAR[deg]; }
        return f * std::pow(2.0, (tun + (kte ? keyErr(n) : 0.0)) / 1200.0);
    }
    double noteFreq(int n) const { return tuneHz(n) * std::pow(2.0, std::lround(p[oct])); }
    double cutHz(double v) const { return 30 * std::pow(18000.0 / 30.0, v); }
    double baseCut() const { return std::min(cutHz(p[cut]) * std::pow(2, (p[kts] > 0.5f ? std::round(p[kt] * 3.0) / 3.0 : (double)p[kt]) * ((lastNote + std::lround(p[oct]) * 12) - 24) / 12.0), 20000.0); }
    double susF() const { return std::min(baseCut() * velBright * std::pow(2, p[fenv] * p[fs]), 20000.0); }
    double tcCoef(double tc) const { return 1 - std::exp(-1 / (std::max(tc, 1e-5) * sr)); }

    // ----- eventos
    void velApply(int vel) {
        double s = velSens, v = std::max(1, std::min(127, vel)) / 127.0, vr = 100 / 127.0;
        static const double VE[3] = { 1.1, 1.7, 2.4 };
        const int vc = std::max(0, std::min(2, vcv));
        auto curve = [s, vc](double x) { return (1 - s) + s * std::pow(x, VE[vc]); };
        velBright = velDest == 1 ? 1 : std::pow(2, s * (v - vr) * 2.4);
        velGainT = velDest == 2 ? 0.883 : 0.883 * curve(v) / curve(vr);
    }
    void triggerEnv(int vel) {
        velApply(vel);
        double att = std::max((double)p[a], 0.002), fat = std::max((double)p[fa], 0.002);
        ast = ATT; astep = (1 - aval) / (att * sr); atc = att * sr; alin = 0;     // alin conta amostras do ataque
        aAttLeft = att * sr;
        fe = 0; feStep = 1.0 / (fat * sr);
        double base = baseCut();
        cutNow = std::min(base * velBright, 20000.0);
        cst = ATT; cTopLeft = fat * sr; cstep = (std::min(base * velBright * std::pow(2, p[fenv]), 20000.0) - cutNow) / cTopLeft;
        // pitch
        if (p[pa] > 0.05f) { pitchC = p[pa] * 100; pitchK = tcCoef(std::max((double)p[pd], 0.01) / 3); } else { pitchC = 0; pitchK = 0; }
        gate = true;
    }
    double aAttLeft = 0, cTopLeft = 0, aDecLeft = 0, cDecLeft = 0, aRelStep = 0;
    void releaseEnv() {
        ast = REL; cst = REL;
        if (p[ec] > 0.5f) feStep2 = fe / (std::max((double)p[fr], 0.02) * sr);
        if (p[ec] > 0.5f) { aRelStep = aval / (std::max((double)p[r], 0.02) * sr) ; aDecLeft = std::max((double)p[r], 0.02) * sr;
            double base = std::max(baseCut(), 5.0); cDecLeft = std::max((double)p[fr], 0.02) * sr; cmul = std::pow(base / std::max(cutNow, 5.0), 1 / cDecLeft); }
        gate = false;
    }
    void setPitch(int n, bool immediate) { freqT = noteFreq(n); glideC = tcCoef(p[glide] / 3); if (immediate || p[glide] < 0.002f) freq = freqT; }
    int pickNote() const {
        if (stack.empty()) return -1;
        if (prio == 1) return *std::min_element(stack.begin(), stack.end());
        if (prio == 2) return *std::max_element(stack.begin(), stack.end());
        return stack.back();
    }
    void trackFilter() { cst = DEC; ctarget = -1; ccoef = tcCoef(0.03); cexp = false; }   // fe acompanha em DEC (não exponencial)
    void coreOn(int n, int vel) {
        held.erase(std::remove(held.begin(), held.end(), n), held.end());
        stack.erase(std::remove(stack.begin(), stack.end(), n), stack.end()); stack.push_back(n);
        if (gate && pickNote() != n) return;
        lastNote = n;
        if (!gate) { setPitch(n, true); newNote(); triggerEnv(vel); }
        else if (p[leg] < 0.5f) { setPitch(n, false); newNote(); triggerEnv(vel); }
        else { setPitch(n, false); velApply(vel); trackFilter(); }
    }
    void realOff(int n) {
        stack.erase(std::remove(stack.begin(), stack.end(), n), stack.end());
        if (stack.empty()) releaseEnv();
        else { int t = pickNote(); if (t != lastNote) { lastNote = t; setPitch(t, false); trackFilter(); } }
    }
    void noteOn(int n, int vel) { coreOn(n, vel <= 0 ? 100 : vel); }
    void noteOff(int n) { if (sustainOn) { held.push_back(n); return; } realOff(n); }
    void setSustain(bool on) {
        if (on == sustainOn) return; sustainOn = on;
        if (!on) { auto hs = held; held.clear(); for (int n : hs) realOff(n); }
    }
    void allOff() { stack.clear(); held.clear(); sustainOn = false; if (gate) releaseEnv(); }

    // ----- osciladores
    static inline double blep(double t, double dt) {
        if (t < dt) { t /= dt; return t + t - t * t - 1; }
        if (t > 1 - dt) { t = (t - 1) / dt; return t * t + t + t + 1; }
        return 0;
    }
    static inline double osc(int w, double ph, double dt, double pwv) {
        if (w == 0) return 1 - 4 * std::fabs(ph - 0.5);
        if (w == 1) return 2 * ph - 1 - blep(ph, dt);
        if (w == 3) return 1 - 2 * ph + blep(ph, dt);
        if (w == 4) return 0.5 * (1 - 4 * std::fabs(ph - 0.5) + 2 * ph - 1 - blep(ph, dt)) * 1.25;
        double q = ph + 1 - pwv; if (q >= 1) q -= 1;
        return (ph < pwv ? 1 : -1) + blep(ph, dt) - blep(q, dt) + (1 - 2 * pwv);
    }

    // ----- processamento (mono). Escreve n amostras em out.
    void process(float* out, int n, const float* in = nullptr) {
        const bool rgOn = p[rg] > 0.5f; const int unN = std::max(0, std::min(3, (int)std::lround(p[un]))); const double ug = 1.0 / std::sqrt(1.0 + unN); double ur[3] = { 1, 1, 1 };
        for (int k2 = 0; k2 < unN; k2++) { double off = (k2 % 2 ? 1 : -1) * (double)(k2 / 2 + 1) / std::ceil((unN + 1) / 2.0) * p[ud] * 40.0; ur[k2] = std::pow(2.0, off / 1200.0); }
        const bool mmOn = p[mm] > 0.5f;
        const int lsr_ = mmOn ? 0 : (int)std::lround(p[lsr]);
        const bool ext = lsr_ > 0 || mmOn;
        const bool mo_ = p[mo] > 0.5f, mf_ = p[mf] > 0.5f, mbLfo = p[mb] > 0.5f;
        const int maEG = p[ma] > 0.5f ? 1 : 0;
        const double md = std::min(1.0, (double)mod + (atDest == 1 ? atVal : 0.0)), gA = std::min(1.0, 2.0 * (1 - p[mx])), gB = std::min(1.0, 2.0 * p[mx]);
        const double xrel = std::exp(-1 / (0.06 * sr)), xl = xlvl, xth = xthr;
        const double lvl1 = p[lvl] * (p[e1] > 0.5f), lvl2 = p[l2] * (p[e2] > 0.5f), lvlS = p[sub], lvlN = p[nz], lvl3 = p[l3] * (p[e3] > 0.5f);
        const int w1 = (int)std::lround(p[wave]), w2_ = (int)std::lround(p[aura::w2]), w3_ = (int)std::lround(p[aura::w3]);
        const double ratio = std::pow(2, std::lround(p[o2]) + p[d2] / 12.0), ratio3 = std::pow(2, std::lround(p[o3]) + p[d3] / 12.0);
        const double fx3 = p[lr] * std::pow(2, p[d3] / 12.0);
        const bool k3on = p[k3] > 0.5f, sync_ = p[sync] > 0.5f, sm = p[sw] > 0.5f, nc_ = p[nc] > 0.5f, ecb = p[ec] > 0.5f;
        const int pol_ = (int)std::lround(p[pol]);
        if (mfirst) { mfirst = false; ml1 = lvl1; ml2 = lvl2; mls = lvlS; mnz = lvlN; ml3 = lvl3; movl = p[ovl]; mres = p[res]; mdrv = p[drv]; }
        const double bs = 1 - std::pow(0.65, 1.0 / 128.0);
        const double lsm = 0.004 * 44100 / sr * 1.0;   // suavização dos níveis (no navegador: 0,004 por amostra)
        const double lsm2 = 0.004;
        (void)lsm;
        const double bendC = bend >= 0 ? bend * bu * 100.0 : bend * bd * 100.0;
        const double lfoGp = mmOn ? 0.0 : ((lsr_ > 0 ? 0 : p[lp] * 100.0) + mod * 60.0 + (atDest == 1 ? atVal * 80.0 : 0.0));
        const double lfoGc = ext ? 0 : (double)p[lf];
        const double lfoGw = ext ? 0 : p[lw] * 0.4;   // ext inclui MOD MIX
        const double atCut = atDest == 2 ? atVal * 1.5 : 0.0;
        const double masterT = p[vol] * (1 + (atDest == 3 ? atVal * 0.6 : 0.0));
        const double mc = tcCoef(0.012), vc = tcCoef(0.004);
        const double lfoInc = p[lr] / sr;
        const bool lfoSq = p[lwv] > 0.5f;
        const double dtT = p[dt], dfT = p[df], dmT = p[dm] * 0.9;
        const double ec_k = tcCoef(0.05), ef_k = tcCoef(0.03);
        const double sl = 1.0 / sr;
        (void)sl;
        const double ampTc = tcCoef(std::max((double)p[d], 0.02) / 3), fTc = tcCoef(std::max((double)p[fd], 0.02) / 3);
        const double arTc = tcCoef(std::max((double)p[r], 0.02) / 3), frTc = tcCoef(std::max((double)p[fr], 0.02) / 3);
        const double sus = p[s];
        const double base = baseCut();

        for (int i = 0; i < n; i++) {
            // --- envelope de volume
            if (ast == ATT) {
                aval += astep; aAttLeft -= 1;
                if (aAttLeft <= 0) { aval = 1; ast = DEC; if (ecb) { double tg = std::max(sus, 0.0001); aDecLeft = std::max((double)p[d], 0.02) * sr; astep = (tg - 1) / aDecLeft; atarget = tg; } else atarget = sus; }
            } else if (ast == DEC) {
                if (ecb) { if (aDecLeft > 0) { aval += astep; aDecLeft -= 1; } else aval = atarget; }
                else aval += (sus - aval) * ampTc;
            } else if (ast == REL) {
                if (ecb) { if (aDecLeft > 0) { aval -= aRelStep; aDecLeft -= 1; if (aval < 0) aval = 0; } else aval = 0; }
                else aval += (0 - aval) * arTc;
            }
            // --- envelope de filtro
            if (cst == ATT) {
                cutNow += cstep; cTopLeft -= 1;
                if (cTopLeft <= 0) {
                    cst = DEC; double sf = susF();
                    if (ecb) { cDecLeft = std::max((double)p[fd], 0.02) * sr; cmul = std::pow(std::max(sf, 5.0) / std::max(cutNow, 5.0), 1 / cDecLeft); cexp = true; }
                    else { cexp = false; ctarget = -1; ccoef = fTc; }
                }
            } else if (cst == DEC) {
                if (cexp) { if (cDecLeft > 0) { cutNow *= cmul; cDecLeft -= 1; } }
                else { double tg = susF(); cutNow += (tg - cutNow) * ccoef; }
            } else if (cst == REL) {
                if (ecb) { if (cDecLeft > 0) { cutNow *= cmul; cDecLeft -= 1; } }
                else cutNow += (base - cutNow) * frTc;
            } else cutNow = base;
            // espelho normalizado do envelope do filtro (fonte de modulação)
            if (cst == ATT) fe = std::min(1.0, fe + feStep);
            else if (cst == DEC) { if (cexp) fe += (p[fs] - fe) * (1.0 / std::max(1.0, cDecLeft + 1)); else fe += (p[fs] - fe) * ccoef; }
            else if (cst == REL) { if (ecb) { fe = std::max(0.0, fe - feStep2); } else fe += (0 - fe) * frTc; } else fe = 0;
            if (ast == REL && aval < 1e-6 && !gate) { aval = 0; }

            // --- pitch / glide
            freq += (freqT - freq) * (p[glide] < 0.002f ? 1.0 : glideC);
            double pc = pitchC; if (pitchK > 0) { pitchC += (0 - pitchC) * pitchK; if (std::fabs(pitchC) < 1e-6) pitchC = 0; }
            // --- LFO nativo
            lfoPh += lfoInc; if (lfoPh >= 1) lfoPh -= 1;
            double lv = lfoSq ? (lfoPh < 0.5 ? 1.0 : -1.0) : (lfoPh < 0.5 ? 4 * lfoPh - 1 : 3 - 4 * lfoPh);

            // --- voz
            ml1 += lsm2 * (lvl1 - ml1); ml2 += lsm2 * (lvl2 - ml2); mls += lsm2 * (lvlS - mls); mnz += lsm2 * (lvlN - mnz); ml3 += lsm2 * (lvl3 - ml3);
            if ((i & 127) == 0) { mres += bs * 0 + 0.35 * (p[res] - mres); mdrv += 0.35 * (p[drv] - mdrv); movl += 0.35 * (p[ovl] - movl); }
            const double ovg = movl * 0.6, k = mres * 4.25, dg = 1 + mdrv * 7, comp = (p[fk] > 0.5f ? 1 + k * 0.2 : 1 + k * 0.55), og = 1 / (1 + 1.5 * mdrv);
            q2p += p[l2r] / sr; if (q2p >= 1) { q2p -= 1; q2s = rnd() * 2 - 1; } q2t += 1.0 / sr;
            const int lw2 = (int)std::lround(p[l2w]);
            const double L2 = (lw2 == 0 ? 1 - 4 * std::fabs(q2p - 0.5) : (lw2 == 1 ? 2 * q2p - 1 : (lw2 == 2 ? (q2p < 0.5 ? 1.0 : -1.0) : q2s))) * (p[l2d] > 0 ? std::min(1.0, q2t / p[l2d]) : 1.0);
            double cents = bendC + lv * lfoGp + pc + prC + L2 * p[l2p] * 200.0; if (lsr_ > 0) cents += mv * p[lp] * 100; else if (mmOn && mo_) cents += mvm * md * (100 + p[lp] * 100);
            double f = freq; if (cents != 0) f *= std::pow(2, cents / 1200);
            const double dt1 = std::min(f / sr, 0.45), dt2 = std::min(f * ratio / sr, 0.45);
            const double pm = fe * p[pwe] * 0.4 + L2 * p[l2m] * 0.4 + lv * lfoGw + (lsr_ > 0 ? mv * p[lw] * 0.4 : 0) + ((mmOn && mo_) ? mvm * md * p[lw] * 0.4 : 0);
            p1 += dt1; bool wrap = false; if (p1 >= 1) { p1 -= 1; wrap = true; }
            p2 += dt2; if (p2 >= 1) p2 -= 1;
            if (wrap && sync_) { p2 = p1 * dt2 / dt1; if (p2 >= 1) p2 -= std::floor(p2); }
            double s3 = 0;
            if (ml3 > 1e-4 || lsr_ == 1 || (mmOn && !maEG)) { double f3 = k3on ? f * ratio3 : fx3, d3_ = std::min(f3 / sr, 0.45); p3 += d3_; if (p3 >= 1) p3 -= 1; s3 = osc(w3_, p3, d3_, 0.5); }
            if (lsr_ == 1) mv = s3; else if (lsr_ == 2) { shT -= p[lr] / sr; if (shT <= 0) { shT += 1; shV = rnd() * 2 - 1; } mv = shV; }
            double sg = 0;
            {   const double pw1v = std::min(0.95, std::max(0.05, p[pw1] + pm)), pw2v = std::min(0.95, std::max(0.05, p[pw2] + pm));
                const double o2s = (ml2 > 1e-4 || rgOn || p[of] > 0.001f) ? osc(w2_, p2, dt2, pw2v) : 0.0; o2sv = o2s;
                if (rgOn) { if (ml1 > 1e-4 || ml2 > 1e-4) sg += std::max(ml1, ml2) * osc(w1, p1, dt1, pw1v) * o2s; }
                else {
                    if (ml1 > 1e-4) { double o1 = osc(w1, p1, dt1, pw1v);
                        if (unN > 0) { for (int k2 = 0; k2 < unN; k2++) { const double dk = std::min(dt1 * ur[k2], 0.45); pu[k2] += dk; if (pu[k2] >= 1) pu[k2] -= 1; o1 += osc(w1, pu[k2], dk, pw1v); } o1 *= ug; }
                        sg += ml1 * o1; }
                    if (ml2 > 1e-4) sg += ml2 * o2s;
                } }
            if (mls > 1e-4) {
                if (sm) { ps += dt1 * 0.5; if (ps >= 1) ps -= 1; sg += mls * 0.75 * ((ps < 0.5 ? 1 : -1) + blep(ps, dt1 * 0.5) - blep(ps < 0.5 ? ps + 0.5 : ps - 0.5, dt1 * 0.5)); }
                else { ps += dt1; if (ps >= 1) ps -= 1; sg += mls * std::sin(TWO_PI * ps); }
            }
            if (ml3 > 1e-4) sg += ml3 * s3;
            if (mnz > 1e-4) { double w = rnd() * 2 - 1, nv = w; if (nc_) { b0 = 0.99765 * b0 + w * 0.0990460; b1 = 0.96300 * b1 + w * 0.2965164; b2 = 0.57000 * b2 + w * 1.0526913; nv = (b0 + b1 + b2 + w * 0.1848) * 0.3; } sg += mnz * 0.6 * nv; }
            if (mmOn) {
                double sa = maEG ? fe : s3, sb;
                if (mbLfo) sb = lv; else { double w = rnd() * 2 - 1; if (p[nc] > 0.5f) { rdN = 0.995 * rdN + 0.1 * w; sb = rdN * 1.4; } else { nn0 = 0.99765 * nn0 + w * 0.0990460; nn1 = 0.96300 * nn1 + w * 0.2965164; nn2 = 0.57000 * nn2 + w * 1.0526913; sb = (nn0 + nn1 + nn2 + w * 0.1848) * 0.3; } }
                mvm = gA * sa + gB * sb;
            }
            if (in && xl > 0) {
                const double xr = in[i]; sg += xr * xl * 2;
                if (xmode) { double a = std::fabs(xr); xe = a > xe ? a : xe * xrel;
                    if (!xtr) { if (xe > xth) { xtr = 1; xh = 0; xNote = lastNote; coreOn(xNote, std::max(30, std::min(127, (int)std::lround(40 + xe * 350)))); } }
                    else if (xe < xth * 0.4) { if (++xh > 2400) { xtr = 0; if (xNote >= 0) { int nn = xNote; xNote = -1; realOff(nn); } } } else xh = 0; }
            }
            sg *= 0.6;
            if (ovg > 1e-4) sg += ovg * hy;
            double cm = L2 * p[l2f] * 2.0 + o2sv * p[of] * 2.0 + lv * lfoGc + atCut + (lsr_ > 0 ? mv * p[lf] : 0.0) + ((mmOn && mf_) ? mvm * md * (1 + p[lf]) : 0.0);
            double fc = cutNow; if (cm != 0) fc *= std::pow(2, cm);
            if (fc > sr * 0.3) fc = sr * 0.3; else if (fc < 5) fc = 5;
            const double g = 1 - std::exp(-TWO_PI * fc / sr);
            const bool acid = p[fk] > 0.5f; const int fmo = (int)std::lround(p[fm]);
            const double x = 1.4286 * std::tanh(0.7 * sg * dg) * comp - k * (acid ? 0.85 : 1.0) * std::tanh(y4) + 1e-9 * (rnd() - 0.5);
            if (acid) { y1 += g * (std::tanh(0.9 * x) - std::tanh(0.9 * y1)) * 1.111; y2 += g * (std::tanh(0.9 * y1) - std::tanh(0.9 * y2)) * 1.111; y3 += g * (std::tanh(0.9 * y2) - std::tanh(0.9 * y3)) * 1.111; y4 += g * (std::tanh(0.9 * y3) - std::tanh(0.9 * y4)) * 1.111; }
            else { y1 += g * (x - y1); y2 += g * (y1 - y2); y3 += g * (y2 - y3); y4 += g * (y3 - y4); }
            double fo; const bool p12 = pol_ <= 2;
            if (acid) fo = y4;
            else if (fmo == 1) fo = p12 ? (y1 - y2) * 4 : (y2 - 2 * y3 + y4) * 16;
            else if (fmo == 2) fo = p12 ? (x - 2 * y1 + y2) : (x - 4 * y1 + 6 * y2 - 4 * y3 + y4);
            else fo = (pol_ == 1 ? y1 : (pol_ == 2 ? y2 : (pol_ == 3 ? y3 : y4)));
            double ov = fo * og;
            hy = ha * (hy + ov - hx); hx = ov;
            if (std::fabs(y4) < 1e-18) y1 = y2 = y3 = y4 = 0;
            if (std::fabs(hy) < 1e-18) hy = 0;

            // --- amp, compressor, velocidade, master
            double sig = ov * aval;
            double ab = std::fabs(sig); ce = ab > ce ? ab : ce * cer;
            if ((cn++ & 3) == 0) cgt = ce > 1e-5 ? sat(ce) / ce : 1;
            cg += (cgt - cg) * (cgt < cg ? cca : ccr);
            sig *= cg * mk;
            velGain += (velGainT - velGain) * vc; sig *= velGain;
            masterG += (masterT - masterG) * mc; sig *= masterG;
            // delay em paralelo
            double dtSm = dtT; (void)dtSm; dtS += (dtT - dtS) * tcCoef(0.05); dfS += (dfT - dfS) * tcCoef(0.03); dmS += (dmT - dmS) * tcCoef(0.03);
            const int L = (int)dline.size();
            double rp = dw - dtS * sr; while (rp < 0) rp += L;
            int i0 = (int)rp; double fr_ = rp - i0; int i1 = i0 + 1 >= L ? 0 : i0 + 1;
            double dv = dline[i0] * (1 - fr_) + dline[i1] * fr_;
            double lp_ = dlb0 * dv + dz1; dz1 = dlb1 * dv - dla1 * lp_ + dz2; dz2 = dlb2 * dv - dla2 * lp_;
            dline[dw] = (float)(sig + dfS * lp_); if (++dw >= L) dw = 0;
            double mix = sig + dmS * lp_;
            // limitador soft (mesma curva do navegador: entrada x/3 sobre uma curva definida em [-3,3])
            preOut = mix;
            const double xp = std::fabs(mix);
            const double outv = xp < 0.6 ? xp : 0.6 + 0.4 * std::tanh((xp - 0.6) / 0.4);
            out[i] = (float)(mix < 0 ? -outv : outv);
        }
    }
};

} // namespace aura
