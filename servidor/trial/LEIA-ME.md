# Servidor do teste grátis (Cloudflare)

Guarda quais e-mails já usaram o teste de 15 dias. Só o resumo SHA-256 do e-mail é gravado, nunca o e-mail em texto.

Banco D1 `aura-trials` (id 8fbf24de-607a-4051-ab41-0d31185c54f3) e a tabela `trials` já foram criados na conta.

## Publicar o Worker (painel do Cloudflare, uns 3 minutos)
1. Workers & Pages → Create → Create Worker. Nome: `aura-trial`. Deploy.
2. Edit code → apague o exemplo, cole o conteúdo de `worker.js` e clique em Deploy.
3. Settings → Bindings → Add → D1 database. Variable name: `DB`. Database: `aura-trials`. Salvar e fazer Deploy.
4. (Recomendado) Settings → Variables and secrets → Add → tipo Secret, nome `TRIAL_SALT`, valor: qualquer texto longo e aleatório. Guarde esse texto: se trocar depois, os e-mails antigos deixam de ser reconhecidos.
5. Abra o endereço do Worker (`https://aura-trial.<seu-subdominio>.workers.dev`) no navegador. Deve aparecer `{"ok":true,"servico":"aura-trial"}`.
6. Informe esse endereço no `trialApi` de `index.html` (comercial). Enquanto `trialApi` estiver vazio, o app não pede e-mail.

## Como funciona
`POST /trial` com `email` e `device`. Se o e-mail nunca foi usado, grava a data de início e responde `new: true`. Se já foi, responde a data original com `new: false`, e o prazo não reinicia. Apelidos do Gmail (pontos e `+algo`) contam como o mesmo e-mail.

Limite: sem confirmação por código, quem digitar um e-mail diferente (inventado) começa um teste novo.
