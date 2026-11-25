# Atualização do AURA Synth Bass sem reinstalar

**Regra de ouro: o usuário decide.** Nada é baixado, aplicado ou reiniciado sem o clique dele, e ele sempre vê o que mudou antes.

**Fluxo**
1. O app procura atualização **uma vez, ao abrir** (4 s depois de abrir). Não há botão de "procurar", nem busca contínua.
2. Se houver versão nova, aparece o ícone **NOVA VERSÃO** no canto inferior direito. A janela não abre sozinha.
3. Ao clicar no ícone, o usuário vê a lista **"o que mudou"** (todas as versões desde a dele, com data e itens) e escolhe **ATUALIZAR AGORA** ou **AGORA NÃO**. Com "agora não" nada muda e o ícone continua lá nas próximas aberturas.
4. Só ao clicar em atualizar: o app baixa a interface nova (já conferida pela assinatura), grava ao lado do app e reinicia.

Presets, setlists, mapeamentos, personalizações e a licença **não são tocados**: ficam no perfil do app.

**"O que mudou" é obrigatório.** O texto vem de `desktop/novidades.json` (a versão mais nova fica no topo: `version` tipo `1.3.0`, `date`, `titulo`, `itens`). O gerador **não cria o pacote** se o topo do arquivo não tiver `itens`. **A cada mudança no app, acrescente uma entrada nova no topo, com versão maior.** Escreva para o músico (o que ele ganha), não para o programador.

**Segurança.** Só vale pacote assinado com a chave privada guardada no segredo `AURA_UPDATE_KEY` do GitHub (chave pública embutida em `src/main.js`). Pacote adulterado, de outra chave ou com nome de arquivo estranho é recusado, na procura e de novo na instalação. A assinatura também é conferida a cada abertura.

**Proteção contra versão ruim.** Se uma versão instalada não chegar a abrir bem 3 vezes seguidas, o app volta sozinho para a interface do instalador e não a oferece de novo.

**Publicação.** Todo push em `comercial` que mude `index.html` (ou `desktop/**`, `vst/**`) roda `.github/workflows/desktop.yml`: gera os instaladores e, no job `atualizacao`, o `aura-update.json` assinado, que vai para a Release mais recente (número = `github.run_number`).

**Quando ainda é preciso reinstalar** (raro): mudança na casca do app (Electron) ou no plugin VST3/AU (compilado). Aí o pacote leva `needsShell` maior que `SHELL_API` do app instalado, e o app mostra o ícone com o texto "instalação nova" e o botão **BAIXAR O INSTALADOR**. Para isso: aumente `SHELL_API` em `src/main.js` e `AURA_NEEDS_SHELL` no workflow.

**Chave de assinatura.** Se perder a chave, gere outra, troque `UPDATE_PUB` em `src/main.js` e publique um instalador novo (os apps antigos só aceitam a chave antiga).
