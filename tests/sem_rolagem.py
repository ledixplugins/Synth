# O AURA nunca pode ter rolagem: em qualquer tamanho de janela o painel inteiro cabe.
from playwright.sync_api import sync_playwright
T=[(1440,900),(1280,720),(1366,768),(1100,650),(1024,600),(1920,1080),(1280,600),(900,500),(2560,1300),(800,450)]
bad=0
with sync_playwright() as p:
    b=p.chromium.launch()
    for w,h in T:
        pg=b.new_page(viewport={'width':w,'height':h}); pg.goto('http://localhost:8765/index.html'); pg.wait_for_timeout(1000)
        r=pg.evaluate("""(()=>{const s=document.querySelector('.synth').getBoundingClientRect();const d=document.documentElement;
          return {b:s.bottom,r:s.right,ih:innerHeight,iw:innerWidth,sy:d.scrollHeight>innerHeight+1||d.scrollWidth>innerWidth+1,ov:getComputedStyle(document.body).overflow}})()""")
        ok=r['b']<=r['ih']+1 and r['r']<=r['iw']+1 and not r['sy']
        print(('ok  ' if ok else 'FALHA'),f'{w}x{h} sem rolagem'); bad+=0 if ok else 1
        # redimensionar depois de aberto
        pg.set_viewport_size({'width':w,'height':max(400,h-120)}); pg.wait_for_timeout(500)
        r=pg.evaluate("(()=>{const s=document.querySelector('.synth').getBoundingClientRect();return s.bottom<=innerHeight+1&&s.right<=innerWidth+1})()")
        print(('ok  ' if r else 'FALHA'),f'{w}x{h} -> encolhendo a janela continua sem rolagem'); bad+=0 if r else 1
        pg.close()
    b.close()
raise SystemExit(1 if bad else 0)
