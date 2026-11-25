// Atualização sem reinstalar: assinatura, integridade, versões, reversão e exigência de nova instalação.
const fs = require('fs'), os = require('os'), path = require('path'), crypto = require('crypto'), { execFileSync } = require('child_process');
const upd = require('../desktop/src/updater');
let falhas = 0; const ok = (c, m) => { console.log((c ? 'ok   ' : 'FALHA ') + m); if (!c) falhas++; };
const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'aura-upd-'));
const par = () => crypto.generateKeyPairSync('ed25519');
const { publicKey, privateKey } = par(), outra = par();
const pub = publicKey.export({ type: 'spki', format: 'pem' }), priv = privateKey.export({ type: 'pkcs8', format: 'pem' });
function gerar(build, needs, chave) {
  const f = path.join(tmp, 'pk' + build + '.json');
  execFileSync('node', [path.join(__dirname, '..', 'tools', 'gerar_atualizacao.js'), f], { env: Object.assign({}, process.env, { AURA_UPDATE_KEY: chave || priv, AURA_BUILD: String(build), AURA_VERSAO: '9.9.' + build, AURA_NEEDS_SHELL: String(needs) }), stdio: 'pipe' });
  return fs.readFileSync(f, 'utf8');
}
(async () => {
  const p5 = gerar(5, 2);
  const v = upd.verificar(p5, pub);
  ok(v && v.build === 5 && v.files['index.html'], 'pacote assinado é aceito');
  ok(!upd.verificar(p5, outra.publicKey.export({ type: 'spki', format: 'pem' })), 'assinatura de outra chave é recusada');
  const m = JSON.parse(p5); const alt = JSON.parse(m.payload); alt.build = 99; ok(!upd.verificar(JSON.stringify({ payload: JSON.stringify(alt), sig: m.sig }), pub), 'pacote alterado depois de assinado é recusado');
  const alt2 = JSON.parse(m.payload); alt2.files['index.html'].b64 = Buffer.from('<script>mal</script>').toString('base64'); const pl2 = JSON.stringify(alt2);
  ok(!upd.verificar(JSON.stringify({ payload: pl2, sig: crypto.sign(null, Buffer.from(pl2), privateKey).toString('base64') }), pub), 'arquivo que não bate com o hash é recusado');
  const alt3 = JSON.parse(m.payload); alt3.files['../fora.txt'] = alt3.files['index.html']; const pl3 = JSON.stringify(alt3);
  ok(!upd.verificar(JSON.stringify({ payload: pl3, sig: crypto.sign(null, Buffer.from(pl3), privateKey).toString('base64') }), pub), 'nome de arquivo com ".." é recusado');
  ok(!upd.verificar('lixo', pub) && !upd.verificar('{}', pub), 'texto inválido é recusado sem erro');

  const base = path.join(tmp, 'web-update'); const buscar = t => async () => t;
  const cfg = (t, extra) => Object.assign({ url: 'x', buscar: buscar(t), base, chavePublica: pub, shellApi: 2, buildAtual: 3 }, extra || {});
  let r = await upd.procurar(cfg(p5)); ok(r.estado === 'pronta' && r.build === 5 && r.version === '9.9.5', 'versão mais nova é baixada (pronta)');
  ok(fs.existsSync(path.join(base, '5', 'index.html')), 'arquivos gravados na pasta da versão');
  ok(upd.raizAtiva(base, pub, 2) === null, 'antes de ativar, a versão baixada ainda não vale');
  ok(upd.ativarPendente(base) === true, 'ativa a versão baixada na próxima abertura');
  let a = upd.raizAtiva(base, pub, 2); ok(a && a.build === 5 && a.dir === path.join(base, '5'), 'versão ativa vem da pasta atualizada');
  upd.confirmarInicio(base);
  r = await upd.procurar(cfg(p5, { buildAtual: 5 })); ok(r.estado === 'nenhuma', 'mesma versão: nada a fazer');
  r = await upd.procurar(cfg(gerar(4, 2), { buildAtual: 5 })); ok(r.estado === 'nenhuma', 'versão mais velha é ignorada');
  r = await upd.procurar(cfg(p5, { buscar: async () => { throw new Error('offline'); } })); ok(r.estado === 'erro', 'sem internet: erro tratado');
  r = await upd.procurar(cfg(gerar(6, 3), { buildAtual: 5 })); ok(r.estado === 'programa', 'pacote que exige casca maior pede nova instalação (e não é aplicado)');
  ok(!fs.existsSync(path.join(base, '6')), 'pacote que exige casca maior não é gravado');
  r = await upd.procurar(cfg(gerar(7, 2, outra.privateKey.export({ type: 'pkcs8', format: 'pem' })), { buildAtual: 5 })); ok(r.estado === 'erro', 'pacote assinado por chave desconhecida é recusado');

  // nova versão baixada com a antiga em uso: a antiga só sai na ativação
  r = await upd.procurar(cfg(gerar(8, 2), { buildAtual: 5 })); ok(r.estado === 'pronta' && fs.existsSync(path.join(base, '5', 'index.html')), 'versão em uso não é apagada durante o download');
  upd.ativarPendente(base); ok(!fs.existsSync(path.join(base, '5')) && fs.existsSync(path.join(base, '8')), 'ao ativar, a versão anterior é removida');
  // adulteração local depois de instalada
  fs.writeFileSync(path.join(base, '8', 'pacote.json'), fs.readFileSync(path.join(base, '8', 'pacote.json'), 'utf8').replace('"sig":"', '"sig":"AAAA'));
  ok(upd.raizAtiva(base, pub, 2) === null, 'pacote adulterado no disco volta para a versão do instalador');
  // reversão: 3 aberturas seguidas sem confirmar
  fs.rmSync(base, { recursive: true, force: true });
  r = await upd.procurar(cfg(gerar(9, 2))); upd.ativarPendente(base);
  ok(upd.raizAtiva(base, pub, 2) && upd.raizAtiva(base, pub, 2) && upd.raizAtiva(base, pub, 2), '3 aberturas sem confirmar ainda usam a versão nova');
  ok(upd.raizAtiva(base, pub, 2) === null, 'na 4ª abertura sem confirmar, volta para a versão do instalador');
  r = await upd.procurar(cfg(gerar(9, 2))); ok(r.estado === 'nenhuma', 'versão que falhou não é baixada de novo');
  r = await upd.procurar(cfg(gerar(10, 2))); upd.ativarPendente(base); upd.raizAtiva(base, pub, 2); upd.confirmarInicio(base);
  let tudo = true; for (let i = 0; i < 6; i++) { tudo = tudo && !!upd.raizAtiva(base, pub, 2); upd.confirmarInicio(base); }
  ok(tudo, 'aberturas que chegam a funcionar nunca perdem a versão nova');
  fs.rmSync(tmp, { recursive: true, force: true });
  console.log(falhas ? '\natualizacao: ' + falhas + ' FALHA(S)' : '\natualizacao: tudo ok'); process.exit(falhas ? 1 : 0);
})();
