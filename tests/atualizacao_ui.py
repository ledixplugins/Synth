# Aviso de atualização na tela do app instalado (simula a ponte do Electron).
from playwright.sync_api import sync_playwright
INIT="""window.__calls=[];window.__cb=null;
window.auraDesktop={deviceId:'x',deviceName:'Mac',version:'1.2.0',config:{},store:{get:async()=>null,set:async()=>true},http:async()=>({ok:true}),openExternal:u=>window.__calls.push(['open',u]),
 update:{ready:async()=>({build:7,version:'1.2.0'}),check:async()=>({}),apply:async()=>{window.__calls.push(['apply']);return true},onStatus:cb=>{window.__cb=cb}}};"""
falhas=[]
def conf(ok,msg):
    print(('ok   ' if ok else 'FALHA ')+msg)
    if not ok: falhas.append(msg)
with sync_playwright() as p:
    b=p.chromium.launch(); pg=b.new_page(viewport={'width':1280,'height':800}); errs=[]
    pg.on('pageerror',lambda e:errs.append(str(e))); pg.add_init_script(INIT+"try{localStorage.setItem('x','1')}catch(e){}"); pg.goto('http://localhost:8765/index.html'); pg.wait_for_timeout(800)
    on=lambda: pg.evaluate("document.getElementById('updBar').classList.contains('on')")
    conf(not on(),'aviso fica escondido enquanto não há atualização')
    pg.evaluate("__cb({estado:'pronta',build:8,version:'1.3.0'})"); pg.wait_for_timeout(100)
    t=pg.inner_text('#updBar'); conf(on() and 'Atualização pronta' in t and 'presets' in t,'atualização pronta mostra o aviso e tranquiliza sobre os dados')
    pg.click('#updBar button:has-text("Depois")'); conf(not on(),'"Depois" esconde o aviso')
    pg.evaluate("__cb({estado:'pronta',build:8})"); pg.click('#updBar button:has-text("Aplicar agora")'); conf(pg.evaluate("__calls.some(c=>c[0]==='apply')"),'"Aplicar agora" chama a aplicação')
    pg.evaluate("__cb({estado:'programa',shell:{url:'https://github.com/ledixplugins/Synth/releases/latest'}})"); pg.click('#updBar button:has-text("Baixar")')
    conf(pg.evaluate("__calls.some(c=>c[0]==='open' && c[1].includes('/releases/latest'))"),'nova instalação: botão Baixar abre a página de downloads')
    pg.evaluate("__cb({estado:'nenhuma'})"); conf('mais recente' in pg.inner_text('#updBar'),'busca manual sem novidade informa que está atualizado')
    pg.close(); b.close()
conf(not errs,'sem erros de página: '+str(errs))
import sys; sys.exit(1 if falhas else 0)
