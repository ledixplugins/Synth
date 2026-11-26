# CHURCH SYNTH BASS virou WORSHIP SYNTH BASS: favoritos e setlist salvos com o nome antigo passam para o nome novo.
from playwright.sync_api import sync_playwright
INIT="""try{ if(!sessionStorage.getItem('ren')){ localStorage.clear();
  localStorage.setItem('aura_favs_v1',JSON.stringify(['CHURCH SYNTH BASS','DEEP SUB BASS']));
  localStorage.setItem('aura_setlist_v1',JSON.stringify(['DEEP SUB BASS','CHURCH SYNTH BASS']));
  sessionStorage.setItem('ren','1'); } }catch(e){}"""
ruins=[]
def conf(ok,msg):
    print(('ok   ' if ok else 'ERRO ')+msg)
    if not ok: ruins.append(msg)
with sync_playwright() as p:
    b=p.chromium.launch(); pg=b.new_page(viewport={'width':1280,'height':800}); errs=[]
    pg.on('pageerror',lambda e:errs.append(str(e))); pg.add_init_script(INIT)
    pg.goto('http://localhost:8765/index.html'); pg.wait_for_timeout(800)
    fav=pg.evaluate("JSON.parse(localStorage.getItem('aura_favs_v1'))"); sl=pg.evaluate("JSON.parse(localStorage.getItem('aura_setlist_v1'))")
    conf(fav==['WORSHIP SYNTH BASS','DEEP SUB BASS'],f'favoritos migrados {fav}')
    conf(sl==['DEEP SUB BASS','WORSHIP SYNTH BASS'],f'setlist migrada {sl}')
    pg.click('.brand'); pg.wait_for_timeout(500)
    t=pg.inner_text('#oBig'); conf(t.startswith('01') and 'WORSHIP SYNTH BASS' in t and '★' in t, f'preset nº 1 com nome novo e favorito ({t})')
    conf(not errs, f'sem erros na página {errs}')
    b.close()
raise SystemExit(1 if ruins else 0)
