// Copia o app web (index.html e ícones) da raiz do repositório para desktop/app antes de empacotar.
const fs = require('fs'), path = require('path');
const root = path.join(__dirname, '..', '..'), out = path.join(__dirname, '..', 'app');
fs.rmSync(out, { recursive: true, force: true }); fs.mkdirSync(out, { recursive: true });
for (const f of ['index.html', 'icon.svg', 'icon-192.png', 'icon-512.png', 'icon-180.png', 'manifest.json']) fs.copyFileSync(path.join(root, f), path.join(out, f));
// número da interface que vai dentro do instalador (o app só baixa atualizações com número MAIOR)
let versao = '0'; try { versao = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'novidades.json'), 'utf8'))[0].version; } catch (e) {}
fs.writeFileSync(path.join(out, 'build.json'), JSON.stringify({ build: parseInt(process.env.AURA_BUILD || '0', 10) || 0, version: versao }));
// no app desktop não há service worker (o app já está instalado)
console.log('app preparado em', out);
