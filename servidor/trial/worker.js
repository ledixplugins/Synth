// Servidor do teste grátis dos plugins AURA (Cloudflare Worker + banco D1 "aura-trials").
// © 2026 Thiago R. Galvão. Todos os direitos reservados.
// Guarda só o resumo criptografado (SHA-256) do e-mail, nunca o e-mail em texto. Um e-mail só inicia o teste uma vez
// por produto: se já existe, devolve a data original de início, então o prazo de 15 dias não reinicia.
// A tabela guarda o teste por produto (chave: product + email_hash). O produto é fixo por rota, nunca vem do corpo
// do pedido (senão bastaria trocar o nome do produto para reiniciar o teste). Um Worker só, compartilhado entre os
// produtos AURA: cada um tem a sua rota (/trial para o Synth Bass, /trial/preamp-bass para o Preamp Bass).
const CORS = { 'access-control-allow-origin': '*', 'access-control-allow-methods': 'POST, OPTIONS', 'access-control-allow-headers': 'content-type' };
const json = (o, s = 200) => new Response(JSON.stringify(o), { status: s, headers: { ...CORS, 'content-type': 'application/json' } });

// "A.B+x@gmail.com" e "ab@gmail.com" são a mesma caixa: normaliza para não burlar com apelidos
function normaliza(email) {
  const m = String(email || '').trim().toLowerCase().match(/^([^@\s]+)@([^@\s]+\.[^@\s]{2,})$/);
  if (!m || email.length > 254) return null;
  let l = m[1].split('+')[0], d = m[2];
  if (d === 'googlemail.com') d = 'gmail.com';
  if (d === 'gmail.com') l = l.replace(/\./g, '');
  return l ? l + '@' + d : null;
}
async function sha256(s) {
  const b = await crypto.subtle.digest('SHA-256', new TextEncoder().encode(s));
  return [...new Uint8Array(b)].map(x => x.toString(16).padStart(2, '0')).join('');
}
async function hmac(chave, texto) {
  const k = await crypto.subtle.importKey('raw', new TextEncoder().encode(chave), { name: 'HMAC', hash: 'SHA-256' }, false, ['sign']);
  const b = await crypto.subtle.sign('HMAC', k, new TextEncoder().encode(texto));
  return [...new Uint8Array(b)].map(x => x.toString(16).padStart(2, '0')).join('');
}
const igual = (a, b) => { a = String(a || ''); b = String(b || ''); if (a.length !== b.length) return false; let d = 0; for (let i = 0; i < a.length; i++) d |= a.charCodeAt(i) ^ b.charCodeAt(i); return d === 0; };
async function corpo(req) {
  const t = req.headers.get('content-type') || '';
  try {
    if (t.includes('json')) return await req.json();
    const f = new URLSearchParams(await req.text()); return Object.fromEntries(f);
  } catch (e) { return {}; }
}

async function registrarTeste(req, env, PRODUCT) {
  const b = await corpo(req), e = normaliza(b.email);
  if (!e) return json({ ok: false, error: 'Digite um e-mail válido.' }, 400);
  // Acesso total do dono (vale para qualquer produto): só com o e-mail listado em OWNER_EMAILS E o código secreto
  // OWNER_CODE (ou a prova assinada que o próprio Worker deu antes).
  if (b.owner_code || b.owner_token) {
    const donos = String(env.OWNER_EMAILS || '').split(/[\s,;]+/).map(normaliza).filter(Boolean);
    const dev0 = String(b.device || '').slice(0, 64);
    if (!env.OWNER_CODE || !donos.includes(e)) return json({ ok: false, error: 'Código inválido.' }, 403);
    const tok = await hmac(env.OWNER_CODE, 'dono|' + e + '|' + dev0);
    if (igual(b.owner_code, env.OWNER_CODE) || igual(b.owner_token, tok)) return json({ ok: true, owner: true, owner_token: tok, started_at: Date.now(), new: false });
    return json({ ok: false, error: 'Código inválido.' }, 403);
  }
  const h = await sha256((env.TRIAL_SALT || '') + '|' + e), agora = Date.now(), dev = String(b.device || '').slice(0, 64);
  const ins = await env.DB.prepare('INSERT OR IGNORE INTO trials (product, email_hash, started_at, device) VALUES (?, ?, ?, ?)').bind(PRODUCT, h, agora, dev).run();
  const novo = (ins.meta && ins.meta.changes) === 1;
  const row = await env.DB.prepare('SELECT started_at FROM trials WHERE product = ? AND email_hash = ?').bind(PRODUCT, h).first();
  if (!row) return json({ ok: false, error: 'Não foi possível registrar agora. Tente de novo.' }, 500);
  return json({ ok: true, started_at: row.started_at, new: novo });
}

export default {
  async fetch(req, env) {
    const u = new URL(req.url);
    if (req.method === 'OPTIONS') return new Response(null, { status: 204, headers: CORS });
    if (u.pathname === '/' && req.method === 'GET') return json({ ok: true, servico: 'aura-trial' });
    if (req.method !== 'POST') return json({ ok: false, error: 'Não encontrado.' }, 404);

    if (u.pathname === '/trial') return registrarTeste(req, env, 'synthbass');
    if (u.pathname === '/trial/preamp-bass') return registrarTeste(req, env, 'preampbass');
    return json({ ok: false, error: 'Não encontrado.' }, 404);
  }
};
