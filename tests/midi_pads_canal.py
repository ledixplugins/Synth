# Botão mapeado vale pela nota E pelo canal: o pad (canal 10) troca de preset; a tecla de mesmo número (canal 1) toca a nota.
# O primeiro preset de fábrica é o CHURCH SYNTH BASS.
from playwright.sync_api import sync_playwright
INIT="""window.__in={name:'SMK',onmidimessage:null};
  navigator.requestMIDIAccess=async()=>({inputs:new Map([['0',window.__in]]),onstatechange:null});
  window.__m=(a)=>window.__in.onmidimessage({data:new Uint8Array(a)});
  try{ if(!sessionStorage.getItem('lim')){ localStorage.clear(); sessionStorage.setItem('lim','1'); } }catch(e){}"""
falhas=[]
def conf(ok,msg):
    print(('ok   ' if ok else 'FALHA ')+msg)
    if not ok: falhas.append(msg)
with sync_playwright() as p:
    b=p.chromium.launch(args=['--autoplay-policy=no-user-gesture-required']); pg=b.new_page(viewport={'width':1280,'height':800}); errs=[]
    pg.on('pageerror',lambda e:errs.append(str(e))); pg.add_init_script(INIT); pg.goto('http://localhost:8765/index.html'); pg.wait_for_timeout(400); pg.click('.brand'); pg.wait_for_timeout(600)
    nm=lambda: pg.inner_text('#oBig')
    conf('CHURCH SYNTH BASS' in nm() and nm().startswith('01'),'primeiro preset é o CHURCH SYNTH BASS ('+nm()+')')
    # mapeia "próximo" com um pad: nota 69 no canal 10 (0x99)
    pg.evaluate("localStorage.setItem('aura_act_map_v1',JSON.stringify({'n69@10':'next'}))"); pg.reload(); pg.wait_for_timeout(400); pg.click('.brand'); pg.wait_for_timeout(600)
    a=nm(); pg.evaluate("__m([0x90,69,100]); __m([0x80,69,0])"); pg.wait_for_timeout(300)
    conf(nm()==a,'tecla A (canal 1, nota 69) não troca o preset')
    pg.evaluate("__m([0x99,69,100]); __m([0x89,69,0])"); pg.wait_for_timeout(300)
    conf(nm()!=a,'pad (canal 10, nota 69) troca o preset ('+a+' -> '+nm()+')')
    # mapeamento antigo (sem canal) continua valendo em qualquer canal
    pg.evaluate("localStorage.setItem('aura_act_map_v1',JSON.stringify({n70:'next'}))"); pg.reload(); pg.wait_for_timeout(400); pg.click('.brand'); pg.wait_for_timeout(600)
    a=nm(); pg.evaluate("__m([0x99,70,100]); __m([0x89,70,0])"); pg.wait_for_timeout(300)
    conf(nm()!=a,'mapeamento antigo (sem canal) continua funcionando')
    pg.close(); b.close()
conf(not errs,'sem erros de página')
import sys; sys.exit(1 if falhas else 0)
