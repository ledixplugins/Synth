// Gera chaves de cortesia do AURA Synth. Uso: node tools/gerar_cortesia.js <chave-privada.pem> email1 [email2 ...]
// A chave privada é a mesma do segredo AURA_UPDATE_KEY. Anote o id de cada chave: é ele que cancela a chave depois.
const fs = require('fs'), path = require('path');
const { gerar, verificar } = require(path.join(__dirname, '..', 'desktop', 'src', 'cortesia.js'));
const [pem, ...emails] = process.argv.slice(2);
if (!pem || !emails.length) { console.error('uso: node tools/gerar_cortesia.js <chave.pem> email1 [email2 ...]'); process.exit(1); }
const priv = fs.readFileSync(pem, 'utf8'), pub = require('crypto').createPublicKey(priv).export({ type: 'spki', format: 'pem' });
for (const e of emails) { const g = gerar(e, priv); if (!verificar(g.chave, pub)) throw new Error('falhou a conferência'); console.log(JSON.stringify(g)); }
