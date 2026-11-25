// Atualização do app sem reinstalar: baixa a interface nova (um pacote assinado), confere a assinatura e guarda ao lado do app.
// © 2026 Thiago R. Galvão. Todos os direitos reservados.
const crypto = require('crypto'), fs = require('fs'), path = require('path');

const OK_NOME = /^[A-Za-z0-9_.-]+(\/[A-Za-z0-9_.-]+)*$/;     // só nomes simples, sem "..", sem caminho absoluto

// Confere a assinatura do pacote e a integridade de cada arquivo. Devolve o conteúdo (objeto) ou null.
function verificar(texto, chavePublica) {
  try {
    const m = JSON.parse(texto);
    if (typeof m.payload !== 'string' || typeof m.sig !== 'string') return null;
    if (!crypto.verify(null, Buffer.from(m.payload), chavePublica, Buffer.from(m.sig, 'base64'))) return null;
    const p = JSON.parse(m.payload);
    if (!Number.isInteger(p.build) || !p.files || typeof p.files !== 'object') return null;
    for (const [nome, f] of Object.entries(p.files)) {
      if (!OK_NOME.test(nome) || nome.split('/').includes('..')) return null;
      const b = Buffer.from(f.b64, 'base64');
      if (crypto.createHash('sha256').update(b).digest('hex') !== f.sha256) return null;
    }
    return p;
  } catch (e) { return null; }
}

const lerJ = f => { try { return JSON.parse(fs.readFileSync(f, 'utf8')); } catch (e) { return null; } };
const gravarJ = (f, j) => { fs.mkdirSync(path.dirname(f), { recursive: true }); const t = f + '.tmp'; fs.writeFileSync(t, JSON.stringify(j)); fs.renameSync(t, f); };

// Pasta da interface atualizada que deve ser usada agora (ou null = usar a que veio no instalador).
// Regras de segurança: reconfere a assinatura a cada abertura; se a abertura anterior com esta versão nunca chegou a
// funcionar (3 aberturas seguidas sem confirmar), descarta a versão e volta para a do instalador.
function raizAtiva(base, chavePublica, shellApi) {
  const cur = lerJ(path.join(base, 'atual.json')); if (!cur || !cur.build) return null;
  if ((cur.tentativas || 0) >= 3) { gravarJ(path.join(base, 'atual.json'), { build: 0, ruins: (cur.ruins || []).concat(cur.build) }); return null; }
  const dir = path.join(base, String(cur.build));
  let txt; try { txt = fs.readFileSync(path.join(dir, 'pacote.json'), 'utf8'); } catch (e) { return null; }
  const p = verificar(txt, chavePublica); if (!p || p.build !== cur.build || (p.needsShell || 0) > shellApi) return null;
  gravarJ(path.join(base, 'atual.json'), Object.assign({}, cur, { tentativas: (cur.tentativas || 0) + 1 }));
  return { dir, build: p.build, version: p.version || '' };
}
function confirmarInicio(base) { const cur = lerJ(path.join(base, 'atual.json')); if (cur && cur.tentativas) { delete cur.tentativas; gravarJ(path.join(base, 'atual.json'), cur); } }

// Procura e baixa uma versão nova. `buscar(url)` devolve o texto do pacote.
// Resultado: {estado:'nenhuma'} | {estado:'pronta', build, version, notes} | {estado:'programa', shell:{version,url}} | {estado:'erro'}
async function procurar({ url, buscar, base, chavePublica, shellApi, buildAtual }) {
  let txt; try { txt = await buscar(url); } catch (e) { return { estado: 'erro', motivo: 'offline' }; }
  const p = verificar(txt, chavePublica); if (!p) return { estado: 'erro', motivo: 'assinatura' };
  const cur = lerJ(path.join(base, 'atual.json')) || {};
  const maior = Math.max(buildAtual || 0, cur.build || 0);
  if (p.build <= maior || (cur.ruins || []).includes(p.build)) return { estado: 'nenhuma' };
  if ((p.needsShell || 0) > shellApi) return { estado: 'programa', shell: p.shell || {}, build: p.build };
  const dir = path.join(base, String(p.build));
  fs.rmSync(dir, { recursive: true, force: true }); fs.mkdirSync(dir, { recursive: true });
  for (const [nome, f] of Object.entries(p.files)) { const d = path.join(dir, nome); fs.mkdirSync(path.dirname(d), { recursive: true }); fs.writeFileSync(d, Buffer.from(f.b64, 'base64')); }
  fs.writeFileSync(path.join(dir, 'pacote.json'), txt);
  gravarJ(path.join(base, 'prox.json'), { build: p.build });           // marcador: será ativada na próxima abertura ou em "Aplicar agora"
  return { estado: 'pronta', build: p.build, version: p.version || '', notes: p.notes || '' };
}
// Ativa a versão baixada (chamado ao abrir o app ou em "Aplicar agora").
function ativarPendente(base) {
  const n = lerJ(path.join(base, 'prox.json')); if (!n || !n.build) return false;
  const cur = lerJ(path.join(base, 'atual.json')) || {};
  gravarJ(path.join(base, 'atual.json'), { build: n.build, ruins: cur.ruins || [] });
  try { fs.rmSync(path.join(base, 'prox.json')); } catch (e) {}
  if (cur.build && cur.build !== n.build) fs.rmSync(path.join(base, String(cur.build)), { recursive: true, force: true });   // só agora a versão anterior sai
  return true;
}
module.exports = { verificar, raizAtiva, confirmarInicio, procurar, ativarPendente };
