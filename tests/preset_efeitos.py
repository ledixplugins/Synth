"""Um preset pode trazer os próprios efeitos.

Confere três coisas: o preset que pede chorus liga o chorus sozinho; o preset
que não pede efeito nenhum desliga o que estava ligado; e um preset antigo,
sem o bloco efx, deixa os efeitos exatamente como o usuário tinha deixado.
"""
from playwright.sync_api import sync_playwright

with sync_playwright() as p:
    b = p.chromium.launch(); pg = b.new_page(viewport={'width': 1280, 'height': 800}); errs = []
    pg.on('pageerror', lambda e: errs.append(str(e)))
    pg.goto('http://localhost:8765/index.html'); pg.wait_for_timeout(900)

    nomes = pg.evaluate("window.__live.presets().map(p=>p.name)")
    seis = ['DRIVE', 'DRIVE (STAGE 2)', 'OCTAVE (CLEAN)', 'OCTAVE (WILD)', 'SUB BASS', 'CHORUS']
    faltando = [n for n in seis if n not in nomes]
    print('presets do vídeo presentes:', not faltando, faltando or '')

    ix = {n: nomes.index(n) for n in seis}
    pg.evaluate(f"window.__live.carregar({ix['CHORUS']})"); pg.wait_for_timeout(200)
    fx_chorus = pg.evaluate("({m:window.__fx.FX.m, d:window.__fx.FX.d, md:window.__fx.FX.md})")
    print('CHORUS liga o chorus:', fx_chorus['m'] == 1 and fx_chorus['d'] == 0, fx_chorus)

    pg.evaluate(f"window.__live.carregar({ix['DRIVE (STAGE 2)']})"); pg.wait_for_timeout(200)
    fx_drive = pg.evaluate("({m:window.__fx.FX.m, d:window.__fx.FX.d, dr:window.__fx.FX.dr})")
    print('DRIVE (STAGE 2) troca para distorção:', fx_drive['d'] == 1 and fx_drive['m'] == 0, fx_drive)

    pg.evaluate(f"window.__live.carregar({ix['DRIVE']})"); pg.wait_for_timeout(200)
    fx_limpo = pg.evaluate("({m:window.__fx.FX.m, d:window.__fx.FX.d})")
    print('DRIVE desliga os dois:', fx_limpo == {'m': 0, 'd': 0}, fx_limpo)

    # preset antigo não mexe nos efeitos que a pessoa deixou ligados
    pg.evaluate("window.__fx.FX.m=1; window.__fx.FX.d=2;")
    pg.evaluate(f"window.__live.carregar({nomes.index('WORSHIP SYNTH BASS')})"); pg.wait_for_timeout(200)
    fx_antigo = pg.evaluate("({m:window.__fx.FX.m, d:window.__fx.FX.d})")
    print('preset antigo não mexe nos efeitos:', fx_antigo == {'m': 1, 'd': 2}, fx_antigo)

    # a oitava abaixo só está ligada onde deve
    sw = pg.evaluate("Object.fromEntries(window.__live.presets().filter(p=>%s.includes(p.name)).map(p=>[p.name,[p.sw,p.sub]]))" % str(seis))
    oitava = {n: v for n, v in sw.items() if v[0] == 1 and v[1] > 0.3}
    print('timbres com oitava abaixo:', sorted(oitava) == ['OCTAVE (CLEAN)', 'OCTAVE (WILD)', 'SUB BASS'], sorted(oitava))

    print('erros de página:', errs)
    b.close()
