// Gera o pacote de atualização assinado (aura-update.json) com a interface atual do app.
// Uso: AURA_UPDATE_KEY="<chave privada PEM>" AURA_BUILD=<número crescente> AURA_VERSAO=1.2.0 node tools/gerar_atualizacao.js [saida.json]
const fs = require('fs'), path = require('path'), crypto = require('crypto');
const raiz = path.join(__dirname, '..');
const chave = process.env.AURA_UPDATE_KEY; if (!chave) { console.error('ERRO: falta o segredo AURA_UPDATE_KEY (chave privada de assinatura).'); process.exit(1); }
const build = parseInt(process.env.AURA_BUILD, 10); if (!(build > 0)) { console.error('ERRO: AURA_BUILD inválido.'); process.exit(1); }
const files = {};
for (const f of ['index.html', 'icon.svg', 'icon-192.png', 'icon-512.png', 'icon-180.png', 'manifest.json']) {
  const b = fs.readFileSync(path.join(raiz, f));
  files[f] = { b64: b.toString('base64'), sha256: crypto.createHash('sha256').update(b).digest('hex') };
}
const payload = JSON.stringify({
  build, version: process.env.AURA_VERSAO || '', needsShell: parseInt(process.env.AURA_NEEDS_SHELL || '2', 10),
  notes: process.env.AURA_NOTAS || '', shell: { version: process.env.AURA_VERSAO || '', url: 'https://github.com/ledixplugins/Synth/releases/latest' }, files
});
const sig = crypto.sign(null, Buffer.from(payload), crypto.createPrivateKey(chave)).toString('base64');
const saida = process.argv[2] || 'aura-update.json';
fs.writeFileSync(saida, JSON.stringify({ payload, sig }));
console.log('Pacote ' + saida + ': build ' + build + ', ' + Object.keys(files).length + ' arquivos, ' + Math.round(fs.statSync(saida).size / 1024) + ' KB');
