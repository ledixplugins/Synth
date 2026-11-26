# Tela de ativação com chave de cortesia (a assinatura é conferida pelo app; aqui o app é simulado).
import json
from playwright.sync_api import sync_playwright
URL='http://localhost:8765/index.html'
BOA='AURA1.c3ludGh8YW1pZ29AZXhlbXBsby5jb218YWJjZDEyMzQ.'+'A'*86
def mk(pg,store,com_cortesia=True):
    pg.add_init_script("""
    window.__store=%s;
    window.auraDesktop={deviceId:'DEV-1',deviceName:'MacBook',version:'1.3.2',config:{api:'https://lic.test/v1/licenses',trialApi:'https://trial.test'},
      store:{get:async k=>window.__store[k]||null,set:async(k,v)=>{window.__store[k]=v}}, openExternal:u=>{}};
    if(%s) window.auraDesktop.cortesia=async k=>k===%s?{produto:'synth',email:'amigo@exemplo.com',id:'abcd1234'}:null;
    """%(json.dumps(store),'true' if com_cortesia else 'false',json.dumps(BOA)))
ruins=[]
def conf(c,m):
    print(('ok   ' if c else 'ERRO ')+m)
    if not c: ruins.append(m)
with sync_playwright() as p:
    b=p.chromium.launch()
    # 1º uso com a chave: tela de e-mail → JÁ TENHO UMA CHAVE → cola → ATIVAR
    pg=b.new_page(); mk(pg,{}); pg.goto(URL); pg.wait_for_function('window.__licReady')
    conf(pg.evaluate('AuraLic.status()')=='email','1º uso pede e-mail do teste')
    pg.click('#licHave'); pg.fill('#licKey','lixo'); pg.click('#licGo'); pg.wait_for_timeout(300)
    conf(pg.evaluate('AuraLic.status()')!='licensed' and pg.inner_text('#licErr')!='', 'chave comum vai ao servidor de licenças e não libera sozinha')
    pg.fill('#licKey','AURA1.xxxxxxxxxx.'+'B'*86); pg.click('#licGo'); pg.wait_for_timeout(300)
    conf('cortesia inválida' in pg.inner_text('#licErr'),'chave de cortesia falsa é recusada com aviso')
    pg.fill('#licKey','  '+BOA[:50]+'\n'+BOA[50:]+' '); pg.click('#licGo'); pg.wait_for_timeout(400)
    conf(pg.evaluate('AuraLic.status()')=='licensed','chave de cortesia ativa (mesmo colada com espaço/quebra)')
    conf(not pg.evaluate("document.getElementById('licOv').classList.contains('on')") and pg.inner_text('#licChip')=='LICENCIADO','janela fecha e selo mostra LICENCIADO')
    store=pg.evaluate('window.__store')
    # reabrir o app: continua licenciado, sem internet e sem prazo
    pg2=b.new_page(); mk(pg2,store); pg2.goto(URL); pg2.wait_for_function('window.__licReady')
    conf(pg2.evaluate('AuraLic.status()')=='licensed','ao reabrir continua licenciado')
    pg2.click('#licChip'); pg2.wait_for_timeout(200)
    conf('amigo@exemplo.com' in pg2.inner_text('#licMsg') and 'cortesia' in pg2.inner_text('#licMsg'),'janela mostra licença de cortesia e o e-mail')
    pg2.click('#licGo'); pg2.wait_for_timeout(300)
    conf(pg2.evaluate('AuraLic.status()')!='licensed','REMOVER DESTE APARELHO tira a licença')
    # licença salva adulterada (app não confirma a chave): perde a licença ao abrir
    bad=json.loads(store['lic']); bad['gift']='AURA1.falsa.'+'C'*86; pg3=b.new_page(); mk(pg3,{'lic':json.dumps(bad)}); pg3.goto(URL); pg3.wait_for_function('window.__licReady')
    conf(pg3.evaluate('AuraLic.status()')!='licensed','licença salva com chave falsa não vale')
    # app antigo (sem a função): avisa para baixar a versão nova
    pg4=b.new_page(); mk(pg4,{},False); pg4.goto(URL); pg4.wait_for_function('window.__licReady'); pg4.click('#licHave'); pg4.fill('#licKey',BOA); pg4.click('#licGo'); pg4.wait_for_timeout(300)
    conf('versão mais nova' in pg4.inner_text('#licErr'),'app antigo pede para baixar a versão nova')
    b.close()
raise SystemExit(1 if ruins else 0)
