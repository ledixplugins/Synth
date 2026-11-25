# Ícone "NOVA VERSÃO" + o que mudou + decisão do usuário (simula a ponte do Electron).
from playwright.sync_api import sync_playwright
INIT="""window.__calls=[];window.__cb=null;window.__apply=true;
window.auraDesktop={deviceId:'x',deviceName:'Mac',version:'1.2.0',config:{},store:{get:async()=>null,set:async()=>true},http:async()=>({ok:true}),openExternal:u=>window.__calls.push(['open',u]),
 update:{ready:async()=>(window.__ready||{build:7,version:'1.2.0',status:null}),apply:async()=>{window.__calls.push(['apply']);return window.__apply},onStatus:cb=>{window.__cb=cb}}};"""
ST="{estado:'disponivel',build:8,version:'1.3.0',notas:[{version:'1.3.0',date:'2026-12-01',titulo:'Terceira',itens:['Novo preset ZAP','Correção do pad']},{version:'1.2.5',date:'2026-11-28',titulo:'Intermediária',itens:['Ajuste do filtro']}]}"
falhas=[]
def conf(ok,msg):
    print(('ok   ' if ok else 'FALHA ')+msg)
    if not ok: falhas.append(msg)
def abre(b,pre=''):
    pg=b.new_page(viewport={'width':1280,'height':800}); errs=[]
    pg.on('pageerror',lambda e:errs.append(str(e))); pg.add_init_script(INIT+pre); pg.goto('http://localhost:8765/index.html'); pg.wait_for_timeout(800); return pg,errs
with sync_playwright() as p:
    b=p.chromium.launch()
    pg,errs=abre(b)
    vis=lambda i: pg.evaluate("(i)=>{const e=document.getElementById(i);return !!e&&getComputedStyle(e).display!=='none'}",i)
    conf(not vis('updBtn') and not vis('updOv'),'sem atualização: nenhum ícone nem janela')
    pg.evaluate("__cb(%s)"%ST); pg.wait_for_timeout(100)
    conf(vis('updBtn') and 'NOVA VERSÃO' in pg.inner_text('#updBtn'),'versão nova: aparece o ícone NOVA VERSÃO')
    conf(not vis('updOv'),'a janela NÃO abre sozinha (só ao clicar no ícone)')
    pg.click('#updBtn'); t=pg.inner_text('#updOv')
    conf('Nova versão 1.3.0' in t and 'Você está na versão 1.2.0' in t,'janela mostra a versão nova e a atual')
    conf('Novo preset ZAP' in t and 'Correção do pad' in t and 'Ajuste do filtro' in t and '01/12/2026' in t,'janela mostra o que mudou em cada versão, com data')
    conf('continuam exatamente como estão' in t,'tranquiliza sobre presets, setlists, mapeamentos e licença')
    conf(not pg.evaluate("__calls.some(c=>c[0]==='apply')"),'abrir a janela não aplica nada')
    pg.click('#updOv button:has-text("AGORA NÃO")'); conf(not vis('updOv') and vis('updBtn'),'"Agora não" fecha a janela e mantém o ícone')
    pg.click('#updBtn'); pg.keyboard.press('Escape'); conf(not vis('updOv'),'Esc fecha a janela')
    pg.click('#updBtn'); pg.click('#updOv button:has-text("ATUALIZAR AGORA")'); pg.wait_for_timeout(100)
    conf(pg.evaluate("__calls.filter(c=>c[0]==='apply').length===1"),'só o clique em ATUALIZAR AGORA aplica')
    pg.close()
    # falha ao atualizar: mensagem e botão volta
    pg,errs2=abre(b,"window.__apply=false;"); pg.evaluate("__cb(%s)"%ST); pg.click('#updBtn'); pg.click('#updOv button:has-text("ATUALIZAR AGORA")'); pg.wait_for_timeout(200)
    conf('Não foi possível atualizar' in pg.inner_text('#updOv') and pg.evaluate("!document.querySelector('#updOv button').disabled"),'se der erro, avisa e permite tentar de novo'); pg.close()
    # aviso já pronto quando a página abre (procura terminou antes da tela carregar)
    pg,errs3=abre(b,"window.__ready={build:7,version:'1.2.0',status:%s};"%ST); conf(pg.evaluate("document.getElementById('updBtn').classList.contains('on')"),'procura feita antes da tela carregar também mostra o ícone'); pg.close()
    # nova instalação do programa
    pg,errs4=abre(b); pg.evaluate("__cb({estado:'programa',version:'2.0.0',shell:{url:'https://github.com/ledixplugins/Synth/releases/latest'},notas:[{version:'2.0.0',itens:['Motor novo de áudio']}]})")
    pg.click('#updBtn'); conf('instalação nova' in pg.inner_text('#updOv') and 'Motor novo de áudio' in pg.inner_text('#updOv'),'mudança de programa: explica e mostra o que mudou')
    pg.click('#updOv button:has-text("BAIXAR O INSTALADOR")'); conf(pg.evaluate("__calls.some(c=>c[0]==='open' && c[1].includes('/releases/latest'))"),'abre a página de downloads'); pg.close()
    # sem a ponte do Electron (navegador comum): nada muda
    pg=b.new_page(); pg.goto('http://localhost:8765/index.html'); pg.wait_for_timeout(600); conf(pg.evaluate("!document.getElementById('updBtn')"),'no navegador comum não existe o recurso'); pg.close()
    b.close()
conf(not (errs or errs2 or errs3 or errs4),'sem erros de página')
import sys; sys.exit(1 if falhas else 0)
