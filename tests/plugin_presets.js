// Confere o plugin VST3/AU com o app: Presets.h tem de ser exatamente o que o gerador produz a partir do index.html
// (mesma ordem e mesmos valores), os parâmetros de arpejo têm de existir no plugin e o preset deve ligar/desligar o arpejador.
const fs=require('fs'), cp=require('child_process'), path=require('path');
const raiz=path.join(__dirname,'..'); const H=path.join(raiz,'vst/Source/Presets.h');
let falhas=0; const f=(m)=>{ console.log('FALHA '+m); falhas++; };
const antes=fs.readFileSync(H,'utf8');
cp.execFileSync('node',[path.join(raiz,'vst/gerar-presets.js')],{cwd:raiz,stdio:'pipe'});
const depois=fs.readFileSync(H,'utf8');
if(antes!==depois){ f('Presets.h está desatualizado em relação ao index.html (rode node vst/gerar-presets.js)'); }
else console.log('ok   Presets.h igual ao gerado a partir do index.html');
const nomesH=[...depois.matchAll(/^\{"([^"]+)",\{/gm)].map(m=>m[1]);
const html=fs.readFileSync(path.join(raiz,'index.html'),'utf8');
const nomesApp=[...html.slice(html.indexOf('const FACTORY = ['),html.indexOf('const FAMILIAS')).matchAll(/^    F\(\{name:'((?:[^'\\]|\\.)*)'/gm)].map(m=>m[1].toUpperCase());
if(nomesH.length!==36 || nomesApp.length!==36) f(`esperava 36 presets, plugin ${nomesH.length}, app ${nomesApp.length}`);
else if(nomesH.join('|')!==nomesApp.join('|')) f('ordem ou nomes diferentes entre plugin e app');
else console.log('ok   36 presets, mesma ordem e mesmos nomes no app e no plugin');
if(new Set(nomesApp).size!==nomesApp.length) f('nomes de fábrica repetidos');
const cpp=fs.readFileSync(path.join(raiz,'vst/Source/PluginProcessor.cpp'),'utf8');
for(const id of ['arpon','arppat','arpdiv','arpoct','arpgate','arpsw','arpbpm']) if(!cpp.includes('"'+id+'"')) f('parâmetro de arpejo ausente no plugin: '+id);
if(!/temArp[\s\S]{0,200}applyArp/.test(cpp)) f('o plugin não aplica o arpejo do preset');
if(!/arpDoPreset/.test(cpp)) f('o plugin não desliga o arpejo ligado pelo preset');
if(!falhas) console.log('ok   plugin aplica e desliga o arpejo do preset');
console.log(falhas?`problemas: ${falhas}`:'tudo certo'); process.exit(falhas?1:0);
