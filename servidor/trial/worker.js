// AURA Synth Bass — servidor do teste grátis (Cloudflare Worker + banco D1 "aura-trials").
// © 2026 Thiago R. Galvão. Todos os direitos reservados.
// Guarda só o resumo criptografado (SHA-256) do e-mail, nunca o e-mail em texto. Um e-mail só inicia o teste uma vez:
// se já existe, devolve a data original de início, então o prazo de 15 dias não reinicia.
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
async function corpo(req) {
  const t = req.headers.get('content-type') || '';
  try {
    if (t.includes('json')) return await req.json();
    const f = new URLSearchParams(await req.text()); return Object.fromEntries(f);
  } catch (e) { return {}; }
}

export default {
  async fetch(req, env) {
    const u = new URL(req.url);
    if (req.method === 'OPTIONS') return new Response(null, { status: 204, headers: CORS });
    if (u.pathname === '/' && req.method === 'GET') return json({ ok: true, servico: 'aura-trial' });
    if (u.pathname !== '/trial' || req.method !== 'POST') return json({ ok: false, error: 'Não encontrado.' }, 404);

    const b = await corpo(req), e = normaliza(b.email);
    if (!e) return json({ ok: false, error: 'Digite um e-mail válido.' }, 400);
    const h = await sha256((env.TRIAL_SALT || '') + '|' + e), agora = Date.now(), dev = String(b.device || '').slice(0, 64);
    const ins = await env.DB.prepare('INSERT OR IGNORE INTO trials (email_hash, started_at, device) VALUES (?, ?, ?)').bind(h, agora, dev).run();
    const novo = (ins.meta && ins.meta.changes) === 1;
    const row = await env.DB.prepare('SELECT started_at FROM trials WHERE email_hash = ?').bind(h).first();
    if (!row) return json({ ok: false, error: 'Não foi possível registrar agora. Tente de novo.' }, 500);
    return json({ ok: true, started_at: row.started_at, new: novo });
  }
};
