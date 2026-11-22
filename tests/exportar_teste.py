# No teste grátis (desktop) só os presets criados pelo usuário podem ser exportados; os de fábrica, só com licença.
import json,time,zipfile,sys
from playwright.sync_api import sync_playwright
URL='http://localhost:8765/index.html'; DAY=86400000
def mk(pg,store):
    pg.add_init_script("""window.__store=%s;
    window.auraDesktop={deviceId:'D1',deviceName:'Mac',version:'1.0.0',config:{api:'https://lic.test/v1/licenses',trialApi:''},
      store:{get:async k=>window.__store[k]||null,set:async(k,v)=>{window.__store[k]=v}}, openExternal:u=>{}};"""%json.dumps(store))
KEY='aura_user_presets_v1'
falhas=[]
def chk(n,c):
    print(('OK  ' if c else 'FALHA ')+n)
    if not c: falhas.append(n)
with sync_playwright() as p:
    b=p.chromium.launch(args=['--autoplay-policy=no-user-gesture-required']); t0=int(time.time()*1000)-2*DAY
    def abre(user):
        pg=b.new_page(viewport={'width':1280,'height':800}); mk(pg,{'lic':json.dumps({'t0':t0,'seen':t0+DAY})})
        if user: pg.add_init_script("try{localStorage.setItem('%s',JSON.stringify([{name:'MEU SOM',oct:-1,wave:'triangle',lvl:0.9,cut:0.5,a:0.01,d:0.3,s:0.5,r:0.2,vol:1}]))}catch(e){}"%KEY)
        pg.goto(URL); pg.wait_for_function('window.__licReady'); pg.wait_for_timeout(500); return pg
    # sem presets do usuário: nada exporta
    pg=abre(False); chk('estado: teste',pg.evaluate('AuraLic.status()')=='trial')
    pg.click('#menuBtn'); chk('rótulo mostra "só os seus presets"','só os seus presets' in pg.inner_text('#mExport'))
    got=[]; pg.on('download',lambda d:got.append(d)); pg.click('#mExpSyx'); pg.wait_for_timeout(500); chk('sem presets próprios: nada baixa',not got)
    pg.close()
    # com um preset do usuário: exporta só ele
    pg=abre(True); got=[]; pg.on('download',lambda d:got.append(d)); pg.click('#menuBtn')
    pg.click('#mExpSyx'); pg.wait_for_timeout(800); chk('com preset próprio: baixa um zip',len(got)==1)
    if got:
        path=got[0].path(); names=zipfile.ZipFile(path).namelist(); chk('zip só com o preset do usuário: '+str(names),names==['AURA_MEU_SOM.syx'])
    pg.close(); b.close()
sys.exit(1 if falhas else 0)
