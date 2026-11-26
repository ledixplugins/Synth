# Servidor do teste grátis (Cloudflare)

Guarda quais e-mails já usaram o teste de 15 dias. Só o resumo SHA-256 do e-mail é gravado, nunca o e-mail em texto.
**Compartilhado entre os produtos AURA** (Synth e Preamp Bass): a tabela guarda o teste por produto — chave
(product, email_hash) — e cada produto tem a sua própria rota no Worker, com o produto fixo no código da rota
(nunca vem do corpo do pedido, senão bastaria trocar o nome do produto para reiniciar o teste).

Banco D1 `aura-trials` (id 8fbf24de-607a-4051-ab41-0d31185c54f3) e a tabela `trials` já existem na conta, com a
chave primária `(product, email_hash)`.

## Publicar o Worker (painel do Cloudflare, uns 3 minutos)
1. Workers & Pages → `aura-trial` → Edit code → apague o conteúdo atual, cole o `worker.js` desta pasta e clique em Deploy.
   (Primeira vez: Create → Create Worker → nome `aura-trial` → Deploy, e só depois Edit code.)
2. Settings → Bindings → confirme o binding D1 `DB` → banco `aura-trials` (se ainda não existir, adicione).
3. (Recomendado) Settings → Variables and secrets → Secret `TRIAL_SALT` com um texto longo e aleatório. Guarde esse texto: se trocar depois, os e-mails antigos deixam de ser reconhecidos.
4. Abra `https://aura-trial.ledixplugins.workers.dev` no navegador. Deve aparecer `{"ok":true,"servico":"aura-trial"}`.
5. `trialApi` já aponta para esse endereço tanto no `index.html` do Synth (comercial) quanto no `Source/Licenca.h` do Preamp Bass.

**Sempre que mudar o `worker.js`, republique-o manualmente aqui** — esta sessão não tem uma ferramenta que publique o
código do Worker diretamente, só o editor de código do painel faz isso.

## Como funciona
`POST /trial` com `email` e `device`: registro do AURA Synth (produto fixo `synthbass` no código).
`POST /trial/preamp-bass` com os mesmos campos: registro do AURA Preamp Bass (produto fixo `preampbass`).
`POST /trial/aura-pad` com os mesmos campos: registro do AURA Pad (produto fixo `aurapad`).

Se esse e-mail nunca usou o teste *daquele produto*, grava a data de início e responde `new: true`. Se já usou,
responde a data original com `new: false`, e o prazo não reinicia. Apelidos do Gmail (pontos e `+algo`) contam
como o mesmo e-mail.

Limite: sem confirmação por código, quem digitar um e-mail diferente (inventado) começa um teste novo.

## Acesso total do dono (sem chave nem prazo)
No Worker, em Settings → Variables and secrets, crie:
- `OWNER_EMAILS` (tipo Secret ou Text): o(s) e-mail(s) do dono, separados por vírgula. Vale para as duas rotas.
- `OWNER_CODE` (tipo Secret): um código longo e aleatório que só o dono sabe.

No app do Synth, na tela de e-mail, toque em "acesso do desenvolvedor", digite o e-mail e o código e toque em
INICIAR TESTE GRÁTIS. O selo passa a mostrar ACESSO TOTAL. O app guarda uma prova assinada, ligada ao aparelho, e a
revalida pela internet (tolerância offline de 30 dias). Só o e-mail, sem o código, não libera nada. (O Preamp Bass
ainda não tem esse atalho na interface; o Worker já aceita o mesmo `owner_code`/`owner_token` na rota dele.)
