#  AgroControl - Firmware do Sistema de Irrigação

Este repositório contém o código-fonte (firmware) desenvolvido no PlatformIO para o microcontrolador do sistema de irrigação inteligente. 

Abaixo está a documentação técnica dos três arquivos centrais que operam o hardware e a comunicação com o servidor central.

---

##  1. `platformio.ini` (Configuração do Ambiente)

Arquivo localizado na **raiz do projeto**. Ele é responsável por configurar o ambiente do compilador e gerenciar as dependências automaticamente, dispensando a instalação manual de bibliotecas na IDE.

### Configurações da Placa
* **Plataforma:** `atmelavr`
* **Placa:** `uno` (Arduino Uno)
* **Framework:** `arduino`
* **Monitor Serial:** Velocidade travada em `9600` bauds.

### Dependências (`lib_deps`)
| Biblioteca | Versão | Descrição |
| :--- | :--- | :--- |
| **ArduinoJson** | `^7.0.0` | Serializa e desserializa os dados no formato JSON para comunicação com o servidor FastAPI. |
| **WiFiEsp** | `^2.2.2` | Habilita os comandos AT para utilizar o ESP-01 como uma interface de rede Wi-Fi. |

---

##  2. `include/config.h` (Mapeamento e Credenciais)

Arquivo localizado na pasta **`include/`**. Funciona como o painel central de variáveis estáticas, armazenando as definições de hardware e os dados sensíveis da rede.

### Mapeamento de Pinos Físicos
* `PIN_SENSOR` **(A0):** Leitura analógica do sensor de umidade do solo.
* `PIN_VCC_SENSOR` **(5):** Alimentação dinâmica do sensor (acionado apenas durante a leitura para evitar oxidação).
* `PIN_RELE` **(6):** Acionamento do módulo relé (Bomba d'água).
* `PIN_LED_VERDE` **(7):** Indicador visual de sistema em repouso (bomba desligada).
* `PIN_LED_VERMELHO` **(8):** Indicador visual de sistema em operação (bomba ligada).
* `PIN_BUZZER` **(4):** Alerta sonoro de operação.

### Configurações de Rede
Para que o sistema funcione, as seguintes constantes devem ser preenchidas no código antes do upload:
* `WIFI_SSID`: Nome da sua rede Wi-Fi.
* `WIFI_PASS`: Senha da rede Wi-Fi.
* `SERVER_IP`: Endereço IP local da máquina onde o servidor Python (FastAPI) está rodando.
* `SERVER_PORT`: Porta de comunicação do servidor (padrão: `8000`).

---

##  3. `src/main.cpp` (Lógica Principal)

Arquivo localizado na pasta **`src/`**. Contém a lógica de negócio (C++), unindo o hardware às requisições de rede. O sistema opera como um **Cliente HTTP** ativo (`WiFiEspClient`).

### Estrutura de Memória (`struct Estado`)
O sistema mantém o estado atualizado em uma estrutura contendo:
* Nível de umidade atual (`0` a `100%`).
* Status do relé (`bombaLigada`).
* Modo de operação (`modoAuto`).
* Limite de acionamento configurado no hardware (`limiteSeco`).

### Ciclo de Operação (`loop`)
1. **Temporizador:** A cada 3 segundos (para garantir a estabilidade serial do ESP-01), o sistema executa sua rotina.
2. **Leitura:** Energiza o pino `VCC` do sensor, lê o valor analógico, mapeia para porcentagem e desliga o pino.
3. **Sincronização (GET):** Consulta a rota `/api/status` do FastAPI. Se detectar uma alteração de estado feita pelo usuário via painel web, sobrepõe a lógica local e desativa o modo automático.
4. **Lógica Nativa:** Se o modo automático estiver ativo e a umidade cair abaixo do `limiteSeco`, a bomba é acionada internamente.
5. **Atualização (POST):** Envia um payload JSON para `/api/update` notificando o servidor FastAPI sobre a umidade atual e o status físico da bomba.
6. **Atuadores:** Atualiza os níveis lógicos (`HIGH`/`LOW`) do Relé e dos LEDs de sinalização de acordo com o estado final calculado.