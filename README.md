# PS1/PS2 -> USB (RP2040-Zero), 2 controles

**Português** | [English](README.en.md)

Adaptador USB para **2 controles PS1/PS2 simultâneos** com RP2040 (RP2040-Zero ou Pico).
Cada porta aparece no PC como um gamepad HID separado (reconhecido no `joy.cpl`, nos jogos e
no navegador). Serve para jogar e como bancada de teste/reparo de controles.

- Suporta PS1 digital, DualShock 1 e 2 (analógico) e joystick analógico PS1.
- O DualShock é colocado sozinho em modo analógico; no DualShock 2 a pressão de L2/R2 é lida.
- Teste de vibração: **SELECT + START + L1** (motor pequeno) e **SELECT + START + R1** (motor grande).
- Licença: MIT.

> Status: firmware escrito e com sintaxe verificada, mas **ainda não testado em hardware**.

## Ligação

As duas portas compartilham DATA, CMD, CLK, 3V3 e GND. Só o ATT é separado (como no console).

| Conector (pino) | Função | RP2040-Zero |
|---|---|---|
| 1 (DATA, marrom) | saída do controle | GP4 + **resistor de 1 kΩ para 3V3** (obrigatório) |
| 2 (CMD, laranja) | entrada do controle | GP3 |
| 3 (motores, cinza) | 7,2 V da vibração | 5V/VBUS (só teste, fraca) ou livre |
| 4 (GND, preto) | GND | GND |
| 5 (VCC, vermelho) | alimentação | **3V3** (nunca 5 V) |
| 6 (ATT, amarelo) | seleção | GP5 (porta 1) / GP6 (porta 2) |
| 7 (CLK, azul) | clock | GP2 |
| 8 | não usado | – |
| 9 (ACK, verde) | confirmação | não usado |

Confira a ordem dos pinos do seu conector com o multímetro antes de ligar.
Os pinos ficam em `psx.h` (SCK/CMD/DAT precisam ser SPI0: GP2/GP3/GP4 ou GP18/GP19/GP16).

### Esquema de ligação

Raspberry Pi Pico:

![Ligação ao Raspberry Pi Pico](docs/wiring_pico.png)

RP2040-Zero (opcional):

![Ligação ao RP2040-Zero](docs/wiring_rp2040_zero.png)

As versões vetoriais (SVG) estão em `docs/`.

## Gerar o .uf2

### Opção A: GitHub Actions (sem instalar nada)

1. Crie um repositório e envie estes arquivos (inclusive `.github/`).
2. Abra a aba Actions, execute "build-uf2" (ou apenas faça um push).
3. Baixe o artefato `ps2_usb_rp2040-uf2`; dentro está o `ps2_usb_rp2040.uf2`.

### Opção B: local (Linux/WSL)

```
sudo apt install cmake build-essential gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
git clone --recurse-submodules https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
export PICO_SDK_PATH=~/pico-sdk
cmake -B build -S .
cmake --build build -j
```

O arquivo sai em `build/ps2_usb_rp2040.uf2`.

## Gravar

Segure o botão BOOT da RP2040-Zero, conecte o USB e solte. Ela aparece como um disco (RPI-RP2).
Copie o `.uf2` para ele; a placa reinicia sozinha.

## Mapeamento

Ordem igual ao *Standard Gamepad* dos navegadores e ao padrão usado pelos emuladores (índices a partir de 0,
como no hardwaretester e no SDL; no `joy.cpl` os botões começam em 1).

| Índice | Botão | Índice | Botão |
|---|---|---|---|
| 0 | ✕ (A) | 9 | Start |
| 1 | ○ (B) | 10 | L3 |
| 2 | □ (X) | 11 | R3 |
| 3 | △ (Y) | 12 | D-pad ↑ |
| 4 | L1 | 13 | D-pad ↓ |
| 5 | R1 | 14 | D-pad ← |
| 6 | L2 | 15 | D-pad → |
| 7 | R2 | 16 | **Mode / Analog** (pulso) |
| 8 | Select | | |

- **Analógicos:** esquerdo = eixos X/Y, direito = eixos Z/Rz. Slider/Dial = pressão analógica de L2/R2
  (DualShock 2; nos demais vale 0 ou 255).
- **D-pad:** sai como 4 botões (12–15) e também como hat/POV, para funcionar em emuladores e em jogos antigos.
  Para usar só um: `PAD_DPAD_AS_BUTTONS` / `PAD_DPAD_AS_HAT` em `main.c`.
- **Botão ANALOG do DualShock:** o firmware deixa o modo destravado. Ao apertar ANALOG o controle volta para digital,
  o adaptador volta para analógico na hora e envia um pulso no **botão 16 (Mode)**. No emulador, associe o botão
  "Analog" (alternar analógico) a esse botão. Para travar o analógico como antes: `PSX_LOCK_ANALOG 1` em `psx.h`.

## Teste no navegador (hardwaretester.com/gamepad)

O adaptador é um gamepad HID genérico (campo "Mapping" = n/a): o navegador mostra botões e eixos por índice, mas pode não
desenhar o layout "padrão" de console (isso depende do VID/PID ser conhecido pelo navegador).
Aperte um botão no controle antes, pois o navegador só lista o gamepad depois do primeiro input.

## LED de status (RP2040-Zero)

O LED RGB da placa (GP16) mostra a conexão com o computador:

| Cor | Significado |
|---|---|
| Verde fixo | Conectado ao computador (USB reconhecida), nenhum controle detectado |
| Azul fixo | Conectado ao computador e pelo menos 1 controle PS1/PS2 detectado |
| Amarelo fixo | USB suspensa (computador dormindo) |
| Vermelho piscando | Placa ligada, mas o computador ainda não reconheceu a USB |

Ajustes em `led.h`: `STATUS_LED_LEVEL` (brilho 0–255), `STATUS_LED_SHOW_PADS` (0 = sempre verde),
`STATUS_LED_MODE` (0 = sem LED, 1 = WS2812 da Zero, 2 = LED comum, ex.: Pico em GP25).

## Emuladores (DuckStation, PCSX2)

O adaptador é um gamepad HID genérico. O SDL (usado pelo DuckStation) não tem um perfil pronto para ele, então o
**mapeamento automático falha** ("Nenhum mapeamento genérico foi gerado"). Isso é esperado: associe os botões
manualmente, um a um, na tela de controles, usando a tabela acima. Para cada porta, escolha o dispositivo
(`SDL-0` / `SDL-1`) e aperte o botão no controle para ele ser reconhecido.

Opcional: linha para o `gamecontrollerdb.txt` do SDL (troque `<GUID>` pelo GUID que o programa mostrar; **não testada**):

```
<GUID>,PS1/PS2 Controller,a:b0,b:b1,x:b2,y:b3,leftshoulder:b4,rightshoulder:b5,lefttrigger:b6,righttrigger:b7,back:b8,start:b9,leftstick:b10,rightstick:b11,dpup:b12,dpdown:b13,dpleft:b14,dpright:b15,guide:b16,leftx:a0,lefty:a1,rightx:a2,righty:a3,platform:Windows,
```

## Nome e ordem dos 2 controles

Os dois controles são interfaces do mesmo dispositivo USB (mesmo VID/PID). O Windows dá o **mesmo nome** a
todos os dispositivos com o mesmo VID/PID, então no `joy.cpl` e no SDL os dois aparecem com o mesmo nome. Por
isso o nome agora é neutro ("PS1/PS2 Controller"), sem "Player 1/2". A porta 1 é a interface 0 e a porta 2 a
interface 1; a ordem em que o sistema lista os dois pode variar, então identifique apertando um botão.

> O PID mudou para `0x5054`: o Windows instala como dispositivo novo e deixa de usar o nome antigo salvo.

## Ajustes

- `PSX_BYTE_GAP_US` (psx.h): pausa entre bytes. Se o controle ficar só digital ou instável, aumente para 30–40.
- `PSX_BAUD` (psx.h): clock do barramento. Com fios longos ou ruído, reduza para 125000.
- `USB_VID` / `USB_PID` (usb_descriptors.c): valores de teste do TinyUSB (`0xCafe:0x5054`). Troque se for distribuir.
- `PSX_LOCK_ANALOG`, `PSX_MODE_PULSE_MS`, `PSX_MODE_SETTLE_MS` (psx.h): travar o analógico, duração do pulso do botão Mode e espera antes de voltar para analógico.
- `PAD_DPAD_AS_BUTTONS`, `PAD_DPAD_AS_HAT` (main.c): como o D-pad é enviado.

## Solução de problemas

| Sintoma | Causa provável |
|---|---|
| Nada aparece no `joy.cpl` | Pull-up de 1 kΩ no DATA ausente, VCC fora de 3,3 V, CMD/DATA trocados |
| Só botões, sem analógico | Controle em modo digital (ID 0x41); aumente `PSX_BYTE_GAP_US` |
| Entradas aleatórias | Fios longos ou sem GND em comum; reduza `PSX_BAUD` |
| Vibração fraca | Pino 3 em 5 V; o motor original usa ~7,2 V |

## Protocolo

SPI modo 3 (clock em repouso alto), LSB primeiro, ~250 kHz. Poll: `01 42 00 [motor1] [motor2] ...`;
resposta: `FF, ID, 5A, dados`. IDs: `41` digital, `73` analógico, `79` analógico com pressão, `F3` configuração.
Botões em lógica invertida (0 = pressionado).
