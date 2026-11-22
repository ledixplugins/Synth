# Testes das novidades: exportação .syx/.mdp (ida e volta e bloqueio no teste/demonstração), arpejo ligado pelo preset,
# aviso de som bloqueado no celular, lista de presets por família com busca. Requer o servidor em http://localhost:8765.
import zipfile, glob, os, tempfile
from playwright.sync_api import sync_playwright
URL='http://localhost:8765/index.html'; falhas=[]
def conferir(ok,msg):
    print(('  ok   ' if ok else 'FALHA ')+msg)
    if not ok: falhas.append(msg)
with sync_playwright() as p:
    b=p.chromium.launch(args=['--autoplay-policy=no-user-gesture-required'])
    def nova(init=None,**kw):
        pg=b.new_page(viewport={'width':1280,'height':800},accept_downloads=True,**kw); pg.errs=[]; pg.on('pageerror',lambda e:pg.errs.append(str(e)))
        if init: pg.add_init_script(init)
        pg.goto(URL); pg.wait_for_timeout(900); return pg
    # 1) exportação ida e volta
    for kind,btn in (('syx','mExpSyx'),('mdp','mExpMdp')):
        pg=nova(); pg.evaluate("document.getElementById('menuBtn').click()")
        with pg.expect_download(timeout=8000) as d: pg.evaluate(f"document.getElementById('{btn}').click()")
        z=zipfile.ZipFile(d.value.path()); nomes=[n for n in z.namelist() if n.endswith('.'+kind)]
        conferir(len(nomes)==36 and z.testzip() is None, f'exportar .{kind}: zip com 36 arquivos ({len(nomes)})')
        tmp=tempfile.mkdtemp(); z.extractall(tmp); n0=pg.evaluate("window.__live.presets().length")
        pg.set_input_files('#fileIn',[os.path.join(tmp,n) for n in nomes]); pg.wait_for_timeout(2200)
        conferir(pg.evaluate("window.__live.presets().length")-n0==36 and not pg.errs, f'.{kind}: os 36 voltam para o AURA sem erro')
        pg.close()
    # 2) exportação bloqueada no teste e na demonstração
    pg=nova("window.AuraLic={status:()=>'trial'};")
    baixou=[]; pg.on('download',lambda d:baixou.append(d)); pg.evaluate("document.getElementById('mExpSyx').click()"); pg.wait_for_timeout(1200)
    conferir(not baixou, 'exportar .syx bloqueado no período de teste'); pg.close()
    pg=nova("window.AuraLic={status:()=>'trial'};"); baixou=[]; pg.on('download',lambda d:baixou.append(d)); pg.evaluate("document.getElementById('mExpMdp').click()"); pg.wait_for_timeout(1200)
    conferir(not baixou, 'exportar .mdp bloqueado no período de teste'); pg.close()
    pg=b.new_page(viewport={'width':1280,'height':800},accept_downloads=True); baixou=[]; pg.on('download',lambda d:baixou.append(d)); pg.goto(URL+'?demo=1'); pg.wait_for_timeout(900)
    pg.evaluate("document.getElementById('mExpSyx').click()"); pg.wait_for_timeout(1200); conferir(not baixou,'exportar .syx bloqueado na demonstração'); pg.close()
    # 3) arpejo ligado pelo preset
    pg=nova(); pg.mouse.click(640,300); pg.wait_for_timeout(500)
    idx=lambda n: pg.evaluate("n=>window.__live.presets().findIndex(p=>p.name===n)",n)
    st=lambda: pg.evaluate("document.getElementById('mArpSt').textContent")
    pg.evaluate("document.getElementById('menuBtn').click()")
    s0=st(); pg.evaluate("i=>window.__live.carregar(i)",idx('SUB ARP')); pg.wait_for_timeout(300); s1=st()
    pg.evaluate("i=>window.__live.carregar(i)",idx('SUB')); pg.wait_for_timeout(300); s2=st()
    conferir('LIGADO' in s1 and 'DESLIGADO' in s2, f'Sub Arp liga o arpejador ({s1!r}) e trocar para Sub desliga ({s2!r})')
    pg.evaluate("i=>window.__live.carregar(i)",idx('SUB AGRO ARP')); pg.wait_for_timeout(300); conferir('LIGADO' in st() and 'DESLIGADO' not in st(),'Sub Agro Arp liga o arpejador')
    conferir(not pg.errs, 'sem erros de página nos arpejos'); pg.close()
    # 4) aviso de som bloqueado (áudio que não acorda)
    TRAVA="""(()=>{ const R=AudioContext.prototype.resume; AudioContext.prototype.resume=function(){ return new Promise(()=>{}); };
      Object.defineProperty(AudioContext.prototype,'state',{get(){ return 'suspended'; }}); })();"""
    pg=nova(TRAVA); pg.mouse.click(640,300); pg.wait_for_timeout(1700)
    txt=pg.evaluate("document.getElementById('oled').textContent").upper()
    conferir('SEM SOM' in txt, f'áudio parado mostra o aviso "Sem som?" no visor ({txt.strip()[:40]!r})'); pg.close()
    pg=nova(); pg.mouse.click(640,300); pg.wait_for_timeout(1700)
    conferir('SEM SOM' not in pg.evaluate("document.getElementById('oled').textContent").upper(),'áudio normal não mostra o aviso'); pg.close()
    # 5) lista de presets por família e busca
    pg=nova(); pg.click('.brand'); pg.wait_for_timeout(300); pg.click('#oled'); pg.wait_for_timeout(400)
    grupos=pg.evaluate("[...document.querySelectorAll('#pList .pgrp')].map(e=>e.textContent)"); linhas=pg.evaluate("document.querySelectorAll('#pList .prow').length")
    conferir(grupos==['SUB','DRIVE','OCTAVE','FUNK, POP E ROCK','LEAD','PLUCK','CHORUS E ESPECIAIS'] and linhas==36, f'lista com 7 famílias e 33 presets ({len(grupos)} grupos, {linhas} linhas)')
    pg.fill('#pSearch','drive'); pg.wait_for_timeout(200)
    conferir(pg.evaluate("document.querySelectorAll('#pList .pgrp').length")==0 and pg.evaluate("document.querySelectorAll('#pList .prow').length")==4, 'busca "drive" acha 4 presets e esconde os títulos')
    pg.fill('#pSearch',''); pg.click('#pCats button:text-is("PLUCK")'); pg.wait_for_timeout(200)
    conferir(pg.evaluate("document.querySelectorAll('#pList .pgrp').length")==0,'com filtro de categoria os títulos de família somem'); pg.close()
    b.close()
print('\nnovidades:', 'TUDO CERTO' if not falhas else f'{len(falhas)} FALHA(S)')
raise SystemExit(1 if falhas else 0)
