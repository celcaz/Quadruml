# Temas

Cada tema é uma pasta dentro de `themes/` com um `theme.json` e os arquivos que ele cita. O tema ativo fica em `config.ini` (`tema=ps5`) e o padrão é `ps5`.

| Pasta | Conteúdo | Vai para o git? |
| --- | --- | --- |
| `ps5/` | Tema pessoal 1:1: logos, fonte, ícones e sons da Sony | **Não** (está no `.gitignore`) |
| `modelo/` | Modelo de `theme.json` para começar um tema | Sim |
| `whitelabel/` | Tema para publicar, com recursos próprios (Fase 5) | Sim |

## Como montar o tema PS5

1. Copie `modelo/theme.json` para `ps5/theme.json` (já feito nesta máquina).
2. Coloque os arquivos nas subpastas com os nomes citados no `theme.json`, ou ajuste os nomes no arquivo:
   - `fonts/`: fonte regular, média e negrito
   - `images/`: logo e ícones
   - `sounds/`: navegação, confirmar, voltar, troféu, platina
3. Ajuste cores, tamanhos e tempos conforme as referências.

Qualquer arquivo ausente é ignorado e o app usa o valor padrão de `qml/theme/Theme.qml`.

## Formato do theme.json

| Grupo | Chaves | Unidade |
| --- | --- | --- |
| `colors` | `background`, `backgroundAlt`, `text`, `textMuted`, `focus`, `accent` | Cor hexadecimal |
| `type` | `title`, `body`, `small` | Pixels na base 1920×1080 |
| `motion` | `fast`, `normal`, `slow` | Milissegundos |
| `fonts` | `regular`, `medium`, `bold` | Caminho relativo à pasta do tema |
| `images` | `logo`, ... | Caminho relativo |
| `sounds` | `move`, `confirm`, `back`, `trophy`, `platinum` | Caminho relativo (WAV) |
