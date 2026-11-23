# Teste grátis com e-mail: começa só depois do e-mail, o mesmo e-mail não reinicia o prazo, e sem internet o primeiro passo avisa.
import json,time,sys,urllib.parse as up
from playwright.sync_api import sync_playwright
URL='http://localhost:8765/index.html'; DAY=86400000
def mk(pg,store,dev='DEV-1'):
    pg.add_init_script("""window.__store=%s;
    window.auraDesktop={deviceId:'%s',deviceName:'Mac',version:'1.0.0',config:{api:'https://lic.test/v1/licenses',trialApi:'https://trial.test'},
      store:{get:async k=>window.__store[k]||null,set:async(k,v)=>{window.__store[k]=v}}, openExternal:u=>{}};"""%(json.dumps(store),dev))
def servidor(pg,db,off=False):
    def h(route):
        if route.request.method=='OPTIONS': return route.fulfill(status=204,headers={'access-control-allow-origin':'*','access-control-allow-headers':'*','access-control-allow-methods':'*'})
        if off: return route.abort()
        kv={k:up.unquote_plus(v) for k,v in (x.split('=',1) for x in (route.request.post_data or '').split('&') if '=' in x)}
        if 'owner_code' in kv or 'owner_token' in kv:
            tok='tok-'+kv.get('device','')
            if kv.get('email','').lower()=='dono@exemplo.com' and (kv.get('owner_code')=='segredo' or kv.get('owner_token')==tok):
                return route.fulfill(status=200,content_type='application/json',headers={'access-control-allow-origin':'*'},body=json.dumps({'ok':True,'owner':True,'owner_token':tok,'started_at':int(time.time()*1000),'new':False}))
            return route.fulfill(status=403,content_type='application/json',headers={'access-control-allow-origin':'*'},body=json.dumps({'ok':False,'error':'Código inválido.'}))
        e=kv.get('email','').lower(); l,d=e.split('@'); l=l.split('+')[0]; l=l.replace('.','') if d=='gmail.com' else l; k=l+'@'+d
        novo=k not in db
        if novo: db[k]=int(time.time()*1000)
        route.fulfill(status=200,content_type='application/json',headers={'access-control-allow-origin':'*'},body=json.dumps({'ok':True,'started_at':db[k],'new':novo}))
    pg.route('https://trial.test/**',h)
falhas=[]
def chk(n,c):
    print(('OK  ' if c else 'FALHA ')+n)
    if not c: falhas.append(n)
with sync_playwright() as p:
    b=p.chromium.launch(args=['--autoplay-policy=no-user-gesture-required']); db={}
    def abre(store,dev='DEV-1',off=False,d=db):
        pg=b.new_page(viewport={'width':1280,'height':800}); mk(pg,store,dev); servidor(pg,d,off); pg.goto(URL); pg.wait_for_function('window.__licReady'); pg.wait_for_timeout(300); return pg
    ov=lambda pg:pg.evaluate("document.getElementById('licOv').classList.contains('on')")
    # 1) primeiro uso: pede o e-mail e bloqueia
    pg=abre({}); chk('1º uso: pede e-mail e o app fica bloqueado',pg.evaluate('AuraLic.status()')=='email' and ov(pg) and not pg.evaluate("document.getElementById('licMail').hidden"))
    pg.fill('#licMail','isso-nao-e-email'); pg.click('#licGo'); pg.wait_for_timeout(200); chk('e-mail inválido é recusado','válido' in pg.inner_text('#licErr') and pg.evaluate('AuraLic.status()')=='email')
    pg.fill('#licMail','Thiago.Teste+a@Gmail.com'); pg.click('#licGo'); pg.wait_for_timeout(500)
    chk('e-mail válido: começa o teste com 15 dias',pg.evaluate('AuraLic.status()')=='trial' and 'TESTE GRÁTIS · 15 DIAS' in pg.inner_text('#licChip') and not ov(pg))
    saved=pg.evaluate("window.__store.lic"); pg.close()
    # 2) reabrir no mesmo aparelho: continua, sem pedir de novo
    pg=abre({'lic':saved}); chk('reabrir: continua em teste, sem pedir e-mail',pg.evaluate('AuraLic.status()')=='trial' and not ov(pg)); pg.close()
    # 3) outro aparelho, mesmo e-mail (com apelidos) e o teste já passou de 15 dias no servidor
    for k in db: db[k]=int(time.time()*1000)-16*DAY
    pg=abre({},dev='DEV-2'); pg.fill('#licMail','thiagoteste@gmail.com'); pg.click('#licGo'); pg.wait_for_timeout(500)
    chk('mesmo e-mail não reinicia: expirado e avisa',pg.evaluate('AuraLic.status()')=='expired' and 'já usou o teste' in pg.inner_text('#licMsg') and ov(pg)); pg.close()
    # 4) e-mail novo no mesmo servidor: teste novo
    pg=abre({},dev='DEV-3'); pg.fill('#licMail','outra.pessoa@exemplo.com'); pg.click('#licGo'); pg.wait_for_timeout(500)
    chk('e-mail novo começa teste de 15 dias',pg.evaluate('AuraLic.status()')=='trial' and '15 DIAS' in pg.inner_text('#licChip')); pg.close()
    # 4b) instalação antiga com teste em andamento (sem e-mail): pede o e-mail e mantém a data mais antiga
    t0=int(time.time()*1000)-3*DAY; pg=abre({'lic':json.dumps({'t0':t0,'seen':t0+DAY})},dev='DEV-4'); chk('instalação antiga sem e-mail: pede e-mail',pg.evaluate('AuraLic.status()')=='email')
    pg.fill('#licMail','antigo@exemplo.com'); pg.click('#licGo'); pg.wait_for_timeout(500); chk('mantém a data mais antiga (restam 12)','12 DIAS' in pg.inner_text('#licChip')); pg.close()
    # 5) sem internet no primeiro passo
    pg=abre({},dev='DEV-5',off=True); pg.fill('#licMail','offline@exemplo.com'); pg.click('#licGo'); pg.wait_for_timeout(500)
    chk('sem internet: avisa e continua bloqueado','internet' in pg.inner_text('#licErr') and pg.evaluate('AuraLic.status()')=='email'); 
    # 6) "já tenho uma chave" mostra o campo da chave
    pg.click('#licHave'); chk('"Já tenho uma chave" mostra o campo da chave',not pg.evaluate("document.getElementById('licKey').hidden") and pg.evaluate("document.getElementById('licMail').hidden")); pg.close()
    # 7) acesso total do dono: e-mail + código secreto
    pg=abre({},dev='DEV-6'); chk('link "acesso do desenvolvedor" aparece na tela de e-mail',not pg.evaluate("document.getElementById('licOwnerLnk').hidden"))
    pg.click('#licOwnerLnk'); pg.fill('#licMail','dono@exemplo.com'); pg.fill('#licOwner','errado'); pg.click('#licGo'); pg.wait_for_timeout(400)
    chk('código errado é recusado','inválido' in pg.inner_text('#licErr') and pg.evaluate('AuraLic.status()')=='email')
    pg.fill('#licOwner','segredo'); pg.click('#licGo'); pg.wait_for_timeout(500)
    chk('código certo: acesso total sem chave e sem prazo',pg.evaluate('AuraLic.status()')=='licensed' and 'ACESSO TOTAL' in pg.inner_text('#licChip') and not ov(pg))
    saved=pg.evaluate("window.__store.lic"); pg.close()
    pg=abre({'lic':saved},dev='DEV-6'); chk('reabrir: continua com acesso total',pg.evaluate('AuraLic.status()')=='licensed' and not ov(pg)); pg.close()
    pg=abre({'lic':saved},dev='DEV-7'); chk('prova de outro aparelho é recusada na revalidação: volta a pedir e-mail',pg.evaluate('AuraLic.status()')=='email' and ov(pg)); pg.close()
    pg=abre({},dev='DEV-8'); pg.click('#licOwnerLnk'); pg.fill('#licMail','outra.pessoa@exemplo.com'); pg.fill('#licOwner','segredo'); pg.click('#licGo'); pg.wait_for_timeout(400)
    chk('outro e-mail com o código certo é recusado',pg.evaluate('AuraLic.status()')=='email' and 'inválido' in pg.inner_text('#licErr')); pg.close()
    # 8) quem já está em teste (e-mail registrado) também alcança o acesso do dono pela janela do selo
    t1=int(time.time()*1000)-5*DAY; pg=abre({'lic':json.dumps({'t0':t1,'seen':t1+DAY,'email':'dono@exemplo.com'})},dev='DEV-9')
    chk('em teste: restam 10 dias e a janela não abre sozinha','10 DIAS' in pg.inner_text('#licChip') and not ov(pg))
    pg.click('#licChip'); chk('janela do selo tem o link do dono',not pg.evaluate("document.getElementById('licOwnerLnk').hidden"))
    pg.click('#licOwnerLnk'); pg.fill('#licMail','dono@exemplo.com'); pg.fill('#licOwner','segredo'); pg.click('#licGo'); pg.wait_for_timeout(500)
    chk('dono entra pela janela do selo: acesso total',pg.evaluate('AuraLic.status()')=='licensed' and 'ACESSO TOTAL' in pg.inner_text('#licChip') and not ov(pg)); pg.close()
    b.close()
sys.exit(1 if falhas else 0)
