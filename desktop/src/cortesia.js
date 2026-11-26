// Chaves de cortesia (presente) do AURA Synth. © 2026 Thiago R. Galvão.
// Formato: AURA1.<dados em base64url>.<assinatura Ed25519 em base64url>, dados = "synth|<e-mail>|<id>".
// A assinatura usa a mesma chave privada das atualizações, mas sobre o texto "AURA-CORTESIA|<dados>":
// assim uma assinatura de chave nunca vale como pacote de atualização, e vice-versa.
// Funciona sem internet e sem servidor. Uma chave pode ser cancelada pelo id em REVOGADAS (index.html).
const crypto = require('crypto');
const PREFIXO = 'AURA-CORTESIA|';
function verificar(chave, chavePublica) {
  try {
    const m = /^AURA1\.([A-Za-z0-9_-]{8,400})\.([A-Za-z0-9_-]{80,100})$/.exec(String(chave || '').replace(/\s+/g, ''));
    if (!m) return null;
    const dados = Buffer.from(m[1], 'base64url').toString('utf8'), sig = Buffer.from(m[2], 'base64url');
    if (sig.length !== 64 || !crypto.verify(null, Buffer.from(PREFIXO + dados), chavePublica, sig)) return null;
    const [produto, email, id] = dados.split('|');
    if (produto !== 'synth' || !email || !id) return null;
    return { produto, email, id };
  } catch (e) { return null; }
}
function gerar(email, chavePrivada, id) {
  id = id || crypto.randomBytes(4).toString('hex');
  const dados = 'synth|' + String(email).trim().toLowerCase() + '|' + id;
  const sig = crypto.sign(null, Buffer.from(PREFIXO + dados), chavePrivada);
  return { chave: 'AURA1.' + Buffer.from(dados).toString('base64url') + '.' + sig.toString('base64url'), id, email: dados.split('|')[1] };
}
module.exports = { verificar, gerar };
