Filamento-Pet
Uma máquina que utiliza uma lógica híbrida (Digital/Analógica). Um Arduíno controla o aquecimento com feedback de um termistor, enquanto um circuito independente com NE555 gera o clock para o driver do motor de passo, garantindo a tração mecânica das tiras de garrafa através de uma matriz aquecida.
Extrusora PET — Controle com ESP32

Sistema de controle para extrusora de filamento PET baseado em ESP32. 
Faz o controle automático de temperatura (com histerese) e o acionamento 
de um motor de passo com monitoramento de RPM, exibindo os dados em um 
display LCD.



## Descrição do Projeto

O sistema realiza duas funções principais:

1. **Controle de temperatura** — lê a temperatura por um termopar tipo K 
   (via módulo MAX6675) e liga/desliga a resistência de aquecimento através 
   de um relé, mantendo a temperatura dentro de uma faixa segura (histerese 
   de 235 °C a 245 °C).

2. **Controle do motor de passo** — o motor (acionado por um driver DRV8825) 
   só é habilitado quando a temperatura e a rotação atingem valores mínimos 
   de segurança. A velocidade é definida por um gerador de pulsos NE555, 
   ajustável por potenciômetro, e o RPM real é calculado pelo ESP32 
   considerando a caixa de redução (56,25:1).

Todas as informações são exibidas em tempo real em um display LCD 16x2 I2C.


## Lista de Materiais

| Componente | Descrição |
| ESP32 DevKit | Microcontrolador principal |
| Módulo MAX6675 + termopar tipo K | Leitura de temperatura |
| DRV8825 | Driver do motor de passo (microstepping 1/32) |
| Motor de passo | 200 passos/volta |
| NE555 | Gerador de pulsos (STEP) |
| Potenciômetro 10 kΩ | Ajuste da velocidade do motor |
| Display LCD 16x2 I2C (0x27) | Interface visual |
| Módulo relé 1 canal | Aciona a resistência de aquecimento |
| Regulador 12V → 5V (ex.: LM2596) | Alimentação da lógica |
| Fonte 12V | Alimentação principal |
| Resistores / capacitores | Circuito de tempo do NE555 e divisor de nível |


## Ligações (Pinagem)

### ESP32 → Módulos
| ESP32 | Função | Destino |
|---|---|---|
| GPIO21 | I2C SDA | LCD SDA |
| GPIO22 | I2C SCL | LCD SCL |
| GPIO18 | SPI SCK | MAX6675 SCK |
| GPIO5  | SPI CS  | MAX6675 CS |
| GPIO19 | SPI SO  | MAX6675 SO |
| GPIO4  | Entrada de pulsos (RPM) | NE555 OUT (via divisor de tensão) |
| GPIO27 | Controle SLEEP | DRV8825 SLEEP |
| GPIO26 | Controle relé | Relé IN |

### Alimentação
- **12V** → entrada do regulador e VMOT do DRV8825
- **5V** (saída do regulador) → ESP32, NE555, LCD, MAX6675, relé, VDD do DRV8825
- **GND comum** entre todos os módulos

> ⚠️ **Importante:** a saída do NE555 (5V) deve passar por um divisor de 
> tensão (R1 10k / R2 20k) antes de chegar ao GPIO4, pois o ESP32 opera em 3,3V.

---

## Lógica de Controle

- **Aquecimento (relé):**
  - Liga quando temperatura ≤ 235 °C
  - Desliga quando temperatura ≥ 245 °C
- **Motor de passo (habilita quando):**
  - Temperatura ≥ 225 °C **E**
  - RPM do carretel ≥ 0,13

---

## Manual do Usuário

### Alimentação
Conecte uma fonte de **12V** na entrada (bloco de terminais). O regulador 
gera automaticamente os 5V da lógica.

### Ajuste de velocidade
Gire o **potenciômetro** para variar a frequência de pulsos do NE555, 
alterando a velocidade do motor de passo.

### Leitura do display

- `T:` — temperatura atual em °C
- `[ON]` / `[OFF]` — estado do aquecimento
- `RPM:` — rotação do carretel (mostra `--` quando o motor está desligado)
- Canto direito: `R` = motor rodando / `Z` = motor em repouso (sleep)

Em caso de falha, o display mostra `ERRO: Termopar!` (verifique a conexão 
do termopar).

### Carregando o código
1. Instale a **Arduino IDE** e o suporte à placa ESP32.
2. Instale as bibliotecas:
   - `Wire` (nativa)
   - `LiquidCrystal_I2C`
   - `max6675`
3. Selecione a placa **ESP32 Dev Module**.
4. Compile e envie (velocidade do monitor serial: **115200 bps**).

---

## Arquivos do Repositório

- `extrusora_pet.ino` — código-fonte
- `/docs` — documentação e esquemático
- `/fotos` — imagens da montagem

---

## Autores
Brunna de Oliveira Rodrigues
Rian Carlos da Silva
Projeto desenvolvido para a disciplina [Projeto Eletrônica II] — [UFSC].
