# Atualização do AURA Synth Bass sem reinstalar

**Como funciona.** O app instalado é uma "casca" (Electron) que carrega a interface e o motor de som (`index.html`). Depois de aberto, ele procura (6 s depois e a cada 6 h) o arquivo assinado `aura-update.json` na Release mais recente do GitHub. Se o número da versão for maior:
1. confere a **assinatura** (chave pública embutida em `src/main.js`) e o hash de cada arquivo;
2. guarda a interface nova em `…/AURA Synth Bass/web-update/<número>/`;
3. mostra o aviso "Atualização pronta" (Aplicar agora / Depois). Sem clicar, ela vale na próxima abertura.

Presets, setlists, mapeamentos, personalizações e a licença **não são tocados**: ficam no perfil do app, que a atualização não altera.

**Segurança.** Só vale pacote assinado com a chave privada guardada no segredo `AURA_UPDATE_KEY` do GitHub. Pacote adulterado, de outra chave ou com nome de arquivo estranho é recusado. A assinatura é conferida de novo a cada abertura.

**Proteção contra versão ruim.** Se uma versão baixada não chegar a abrir bem 3 vezes seguidas, o app volta sozinho para a interface do instalador e não baixa essa versão de novo.

**Publicação.** Todo push em `comercial` que mude `index.html` (ou `desktop/**`, `vst/**`) roda `.github/workflows/desktop.yml`: gera os instaladores e, no job `atualizacao`, o `aura-update.json` assinado (número = `github.run_number`).

**Quando ainda é preciso reinstalar** (raro): mudança na casca do app (novo recurso do sistema, motor Electron) ou no plugin VST3/AU (compilado). Aí o pacote leva `needsShell` maior que `SHELL_API` do app instalado, e o app mostra "Nova versão do programa disponível" com o botão Baixar. Para isso: aumente `SHELL_API` em `src/main.js` e `AURA_NEEDS_SHELL` no workflow.

**Chave de assinatura.** Segredo do GitHub: `AURA_UPDATE_KEY` (PEM). Se perder a chave, gere outra, troque `UPDATE_PUB` em `src/main.js` e publique um instalador novo (os apps antigos só aceitam a chave antiga).
