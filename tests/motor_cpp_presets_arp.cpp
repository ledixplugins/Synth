// Verifica, com o motor e o arpejador do plugin (os mesmos arquivos do VST3/AU), os 35 presets de fábrica:
//  1) cada preset toca (não é mudo, não tem NaN, não passa de um nível absurdo);
//  2) só Sub Arp e Sub Agro Arp trazem arpejo, com valores válidos, e o arpejador do plugin toca no andamento do preset.
#include "AuraEngine.h"
#include "Arp.h"
#include "Presets.h"
#include <cstdio>
#include <cstring>
#include <cmath>
int main(){
  int falhas=0, comArp=0;
  for(int pi=0; pi<aura::kNumFactory; pi++){
    const auto& P=aura::kFactory[pi];
    aura::Engine e; e.prepare(48000); for(int k=0;k<aura::NP;k++) e.p[k]=P.v[k]; e.reset(); e.noteOn(48,100);
    double pk=0; bool nan=false; float o;
    for(int i=0;i<48000;i++){ e.process(&o,1); if(!std::isfinite(o)) nan=true; double a=std::fabs(o); if(a>pk) pk=a; }
    if(nan || pk<0.01 || pk>4.0){ printf("FALHA %s: pico %.4f nan=%d\n",P.name,pk,(int)nan); falhas++; }
    bool esperaArp = !strcmp(P.name,"SUB ARP") || !strcmp(P.name,"SUB AGRO ARP");
    if(P.temArp!=esperaArp){ printf("FALHA %s: temArp=%d, esperado %d\n",P.name,(int)P.temArp,(int)esperaArp); falhas++; }
    if(!P.temArp) continue;
    comArp++;
    const float* a=P.arp;   // on, pat, div, oct, gate, sw, bpm
    if(a[0]!=1.f || a[1]<0||a[1]>4 || a[2]<0||a[2]>3 || a[3]<1||a[3]>3 || a[4]<0.1f||a[4]>0.95f || a[5]<0||a[5]>1 || a[6]<40||a[6]>240){ printf("FALHA %s: valores de arpejo fora da faixa\n",P.name); falhas++; continue; }
    aura::Arp ar; ar.on=true; ar.pat=(int)a[1]; ar.div=(int)a[2]; ar.oct=(int)a[3]; ar.gate=a[4]; ar.swing=a[5]; ar.bpm=a[6];
    ar.press(48,100); ar.press(55,100);
    int ons=0; const double sr=48000; const int B=64;
    for(int i=0;i<(int)(2*sr/B);i++) ar.advance(B,sr,[&](int,int){ons++;},[&](int){});
    double passo=60.0/ar.bpm*aura::Arp::DIV[ar.div]; double esperado=2.0/passo;
    if(std::fabs(ons-esperado)>2.5){ printf("FALHA %s: %d notas em 2 s, esperado cerca de %.1f\n",P.name,ons,esperado); falhas++; }
    else printf("ok %s: arpejo %d notas em 2 s (esperado %.1f)\n",P.name,ons,esperado);
  }
  if(comArp!=2){ printf("FALHA: %d presets com arpejo, esperado 2\n",comArp); falhas++; }
  printf("presets: %d, problemas: %d\n",aura::kNumFactory,falhas);
  return falhas?1:0;
}
