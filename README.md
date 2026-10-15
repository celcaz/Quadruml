# Quadrum I

Hub de jogos para Windows com interface 1:1 inspirada no PS5. Reúne jogos de PS1, PS2, PS3, PS4 (via emuladores) e PC numa única biblioteca, com troféus unificados (bronze, prata, ouro e platina), perfis de usuário e saves sincronizados na nuvem.

O nome é "Square One" em latim (*Quadrum Primum*): o primeiro quadrado, o ponto de partida.

## Estado

Fase 1, Etapa 2 concluída: fundação (tela cheia, tema, idiomas, log) e banco de dados com perfis.

## Como compilar

Requisitos: Qt 6.12 (kit MSVC 2022 64-bit), Visual Studio 2022 Build Tools com C++, CMake e Ninja (vêm com o Qt).

Pelo Qt Creator:

1. *Open Project* → `CMakeLists.txt`.
2. Escolha o kit *Desktop Qt 6.12.0 MSVC2022 64bit*.
3. *Build* → *Run* (Ctrl+R).

Pelo terminal: `tools\build.bat` compila em `build\msvc-debug`. Os testes rodam com `ctest --test-dir build\msvc-debug`.

Atalhos de desenvolvimento: F11 alterna tela cheia, F12 mostra o painel de desenvolvimento, Ctrl+L troca o idioma, Ctrl+Q fecha.
Dados e log ficam em `%LOCALAPPDATA%\QuadrumI` (`logs\quadrum.log`).

## Pastas de jogos

Até a tela de Configurações (Etapa 8), as pastas ficam em `%LOCALAPPDATA%\QuadrumI\config.ini`:

```ini
[pastas]
ps2=D:/GAMES/PS2/GAMES
ps3=D:/GAMES/PS3/GAMES

[emuladores]
pcsx2=D:/GAMES/PS2/PCSX2
rpcs3=D:/GAMES/PS3/EMULATOR
```

- **PS2**: arquivos `.iso`. O nome vem do `resources/GameIndex.yaml` do PCSX2 indicado em `emuladores/pcsx2`.
- **PS3**: pastas de jogo (`<jogo>/PS3_GAME/PARAM.SFO`) e `.iso`, até dois níveis dentro da pasta.

## Variáveis de desenvolvimento

- `QUADRUM_SCREENSHOT=caminho.png`: salva uma captura da janela alguns segundos depois de abrir.
- `QUADRUM_DEV_KEYS=Right,Right,Down`: simula teclas antes da captura.

## Documentos

- Documento de projeto (arquitetura, decisões, roadmap): https://claude.ai/code/artifact/66e94dc9-2580-40b5-a4f3-b4999e5e9327
- Wireframes: [docs/wireframes/index.html](docs/wireframes/index.html)

## Tecnologia escolhida

- Qt 6 + QML na interface
- Núcleo em C++20, CMake
- SQLite (Qt SQL), SDL3 (controle), QtMultimedia
- Ludusavi (saves de PC) e rclone (nuvem) como ferramentas externas

## Temas

- `themes/ps5/`: tema pessoal 1:1 com o PS5 (logos, fonte, ícones e sons da Sony). Só local: está no `.gitignore` e nunca entra em instalador ou publicação.
- `themes/whitelabel/`: mesmo layout e animações com recursos próprios. Obrigatório antes de qualquer publicação (Fase 5).

## Emuladores suportados

| Plataforma | Emulador |
| --- | --- |
| PS1 | DuckStation |
| PS2 | PCSX2 |
| PS3 | RPCS3 |
| PS4 | shadPS4 (experimental) |

BIOS, firmware e jogos não fazem parte do projeto: cada usuário usa os dumps dos próprios consoles e discos.
