# Testa o licenciamento do app desktop com um servidor de licenças falso (compatível com o protocolo ativar/validar/desativar).
import json,time,sys
from playwright.sync_api import sync_playwright
URL='http://localhost:8765/index.html'; DAY=86400000
def mk(pg,store,dev='DEVICE-AAAA1111'):
    pg.add_init_script("""
    window.__store=%s;
    window.auraDesktop={deviceId:'%s',deviceName:'MacBook',version:'1.0.0',config:{api:'https://lic.test/v1/licenses',trialApi:''},
      store:{get:async k=>window.__store[k]||null,set:async(k,v)=>{window.__store[k]=v}}, openExternal:u=>{window.__opened=u}};
    """%(json.dumps(store),dev))
def server(pg,state):
    def h(route):
        u=route.request.url; body=route.request.post_data or ''; kv=dict(x.split('=',1) for x in body.split('&') if '=' in x)
        import urllib.parse as up; kv={k:up.unquote_plus(v) for k,v in kv.items()}
        def ans(code,j): route.fulfill(status=code,content_type='application/json',headers={'access-control-allow-origin':'*'},body=json.dumps(j))
        if route.request.method=='OPTIONS': return route.fulfill(status=204,headers={'access-control-allow-origin':'*','access-control-allow-headers':'*','access-control-allow-methods':'*'})
        if u.endswith('/activate'):
            k=kv.get('license_key'); L=state['keys'].get(k)
            if L is None: return ans(404,{'activated':False,'error':'license_key not found.'})
            if len(L)>=3: return ans(400,{'activated':False,'error':'This license key has reached the activation limit.'})
            iid='inst-%d'%(len(L)+1+state['n']); state['n']+=1; L.append(iid); return ans(200,{'activated':True,'instance':{'id':iid},'license_key':{'status':'active'}})
        if u.endswith('/validate'):
            k=kv.get('license_key'); L=state['keys'].get(k)
            return ans(200,{'valid':bool(L is not None and kv.get('instance_id') in L and not state.get('revoked'))})
        if u.endswith('/deactivate'):
            L=state['keys'].get(kv.get('license_key')); i=kv.get('instance_id')
            if L and i in L: L.remove(i); return ans(200,{'deactivated':True})
            return ans(404,{'deactivated':False})
    pg.route('https://lic.test/**',h)
def run():
    res=[]
    def chk(n,c): res.append((n,bool(c))); print(('OK  ' if c else 'FALHA ')+n)
    with sync_playwright() as p:
        b=p.chromium.launch()
        # 1) primeiro uso: teste de 15 dias
        pg=b.new_page(); mk(pg,{}); st={'keys':{'AAA-111':[]},'n':0}; server(pg,st); pg.goto(URL); pg.wait_for_function('window.__licReady'); 
        chk('1º uso: trial com 15 dias',pg.evaluate('AuraLic.status()')=='trial' and 'TESTE GRÁTIS · 15 DIAS' in pg.inner_text('#licChip'))
        chk('1º uso: overlay oculto',not pg.evaluate("document.getElementById('licOv').classList.contains('on')"))
        # 2) 10 dias depois
        t0=int(time.time()*1000)-10*DAY; pg2=b.new_page(); mk(pg2,{'lic':json.dumps({'t0':t0,'seen':t0+DAY})}); server(pg2,st); pg2.goto(URL); pg2.wait_for_function('window.__licReady')
        chk('dia 10: restam 5 dias','5 DIAS' in pg2.inner_text('#licChip'))
        # 3) expirado bloqueia áudio
        t1=int(time.time()*1000)-16*DAY; pg3=b.new_page(); pg3.add_init_script("window.__acn=0;(function(){const A=window.AudioContext;window.AudioContext=function(...a){window.__acn++;return new A(...a)};window.AudioContext.prototype=A.prototype})()"); mk(pg3,{'lic':json.dumps({'t0':t1,'seen':t1})}); server(pg3,st); pg3.goto(URL); pg3.wait_for_function('window.__licReady')
        chk('expirado: overlay visível',pg3.evaluate("document.getElementById('licOv').classList.contains('on')"))
        pg3.mouse.click(600,800); pg3.wait_for_timeout(400)
        chk('expirado: AudioContext não é criado',pg3.evaluate("window.__acn===0"))
        # 4) chave inválida
        pg3.fill('#licKey','ZZZ'); pg3.click('#licGo'); pg3.wait_for_timeout(500)
        chk('chave inválida: mensagem',pg3.inner_text('#licErr').strip()!='')
        # 5) ativa
        pg3.fill('#licKey','AAA-111'); pg3.click('#licGo'); pg3.wait_for_timeout(600)
        chk('ativação: licenciado e overlay fechado',pg3.evaluate('AuraLic.status()')=='licensed' and not pg3.evaluate("document.getElementById('licOv').classList.contains('on')"))
        chk('ativação: instância salva no armazenamento',json.loads(pg3.evaluate("window.__store.lic"))['inst'].startswith('inst-'))
        # 6) limite de 3 aparelhos
        for i in range(2):
            q=b.new_page(); mk(q,{'lic':json.dumps({'t0':t1,'seen':t1})},dev='DEV-%d'%i); server(q,st); q.goto(URL); q.wait_for_function('window.__licReady'); q.fill('#licKey','AAA-111'); q.click('#licGo'); q.wait_for_timeout(500)
            chk('aparelho %d ativado'%(i+2),q.evaluate('AuraLic.status()')=='licensed')
        q=b.new_page(); mk(q,{'lic':json.dumps({'t0':t1,'seen':t1})},dev='DEV-X'); server(q,st); q.goto(URL); q.wait_for_function('window.__licReady'); q.fill('#licKey','AAA-111'); q.click('#licGo'); q.wait_for_timeout(500)
        chk('4º aparelho é recusado',q.evaluate('AuraLic.status()')=='expired' and '3 aparelhos' in q.inner_text('#licErr'))
        # 7) desativar libera vaga
        pg3.click('#licChip'); pg3.wait_for_timeout(200); pg3.click('#licGo'); pg3.wait_for_timeout(600)
        chk('desativar: volta a expirado',pg3.evaluate('AuraLic.status()')=='expired')
        q.click('#licGo'); q.wait_for_timeout(600)
        chk('vaga liberada: 4º aparelho ativa',q.evaluate('AuraLic.status()')=='licensed')
        # 8) licença revogada é bloqueada na revalidação
        lic=json.loads(q.evaluate("window.__store.lic")); st['revoked']=True
        r=b.new_page(); mk(r,{'lic':json.dumps(lic)}); server(r,st); r.goto(URL); r.wait_for_function('window.__licReady'); r.wait_for_timeout(500)
        chk('revogada/reembolsada: bloqueia',r.evaluate('AuraLic.status()')!='licensed')
        st['revoked']=False
        # 9) offline dentro da tolerância
        lic2=dict(lic,val=int(time.time()*1000)-5*DAY); o=b.new_page(); mk(o,{'lic':json.dumps(lic2)}); o.route('https://lic.test/**',lambda rt:rt.abort()); o.goto(URL); o.wait_for_function('window.__licReady'); o.wait_for_timeout(300)
        chk('offline 5 dias: continua licenciado',o.evaluate('AuraLic.status()')=='licensed')
        lic3=dict(lic,val=int(time.time()*1000)-40*DAY); o2=b.new_page(); mk(o2,{'lic':json.dumps(lic3)}); o2.route('https://lic.test/**',lambda rt:rt.abort()); o2.goto(URL); o2.wait_for_function('window.__licReady'); o2.wait_for_timeout(300)
        chk('offline 40 dias: pede revalidação',o2.evaluate('AuraLic.status()')=='recheck')
        # 10) relógio adiantado/atrasado
        fut=int(time.time()*1000)+20*DAY; c=b.new_page(); mk(c,{'lic':json.dumps({'t0':int(time.time()*1000)-5*DAY,'seen':fut})}); server(c,st); c.goto(URL); c.wait_for_function('window.__licReady')
        chk('relógio voltou no tempo: bloqueia',c.evaluate('AuraLic.status()')=='clock')
        # 11) app web sem licenciamento
        w=b.new_page(); w.goto(URL); w.wait_for_timeout(500); chk('versão web: sem licença nem overlay',w.evaluate("typeof window.AuraLic==='undefined'"))
        b.close()
    bad=[n for n,c in res if not c]; print('\n%d/%d ok'%(len(res)-len(bad),len(res))); sys.exit(1 if bad else 0)
run()
