# Um pad que chega por várias portas MIDI (ou repica) deve trocar UM preset por aperto, não vários.
from playwright.sync_api import sync_playwright
INIT="""window.__ins=[1,2,3].map(i=>({name:'SMK '+i,onmidimessage:null}));
  navigator.requestMIDIAccess=async()=>({inputs:new Map(window.__ins.map((x,i)=>[String(i),x])),onstatechange:null});
  window.__tri=(a)=>window.__ins.forEach(x=>x.onmidimessage({data:new Uint8Array(a)}));
  window.__one=(a)=>window.__ins[0].onmidimessage({data:new Uint8Array(a)});
  try{localStorage.clear();localStorage.setItem('aura_act_map_v1',JSON.stringify({n36:'next'}))}catch(e){}"""
falhas=[]
num=lambda t:int(t.split()[0])
def conf(ok,msg):
    print(('ok   ' if ok else 'FALHA ')+msg)
    if not ok: falhas.append(msg)
with sync_playwright() as p:
    b=p.chromium.launch(args=['--autoplay-policy=no-user-gesture-required']); pg=b.new_page(viewport={'width':1280,'height':800}); errs=[]
    pg.on('pageerror',lambda e:errs.append(str(e))); pg.add_init_script(INIT); pg.goto('http://localhost:8765/index.html'); pg.wait_for_timeout(400); pg.click('.brand'); pg.wait_for_timeout(600)
    nm=lambda: pg.inner_text('#oBig')
    def passos(envio):
        a=nm(); pg.evaluate(envio); pg.wait_for_timeout(60); r=nm()
        pg.evaluate("__one([0x80,36,0])"); pg.wait_for_timeout(260); return a,r
    a0=nm()
    a,r=passos("__tri([0x90,36,100])"); conf(num(r)-num(a)==1,'3 portas = 1 passo ('+a+' -> '+r+')')
    pg.click('#pPrev'); pg.wait_for_timeout(250); base=nm()
    pg.evaluate("__one([0x90,36,100]); __one([0x90,36,100]); __one([0x90,36,100])"); pg.wait_for_timeout(60); conf(num(nm())-num(base)==1,'repique x3 = 1 passo')
    d1=nm(); pg.click('#pPrev'); pg.wait_for_timeout(250); x=nm(); pg.evaluate("__one([0x90,36,100])"); pg.wait_for_timeout(260); pg.evaluate("__one([0x90,36,100])"); pg.wait_for_timeout(60)
    conf(num(nm())-num(x)==2,'dois apertos separados = 2 passos')
    pg.close(); b.close()
conf(not errs,'sem erros de página')
import sys; sys.exit(1 if falhas else 0)
