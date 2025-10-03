# IOT_TRAB_01

ALUNO:
GABRIEL HENRIQUE FERNANDES TELLES

## 📊 Fluxo do Projeto ESP32

```mermaid
flowchart TD
    A[Sensores ESP32] -->|Leitura Digital| B[ESP32]

    subgraph "ESP32"
        B -->|Processa Dados| C[Controle de LEDs]
        B -->|Publica JSON| D[Broker MQTT]
        B -->|POST /insert| E[Cloudflare KV]
    end

    %% LEDs
    C --> L1[LED Seco 💡]
    C --> L2[LED Úmido 💧]
    C --> L3[LED Chuvoso + Úmido 🌧️💧]

    %% Cloudflare
    E -->|Armazena Último Valor| F[(KV Store)]
    E -->|Histórico de Leituras| F

    %% HTML
    D -->|Dados Sensores| G[Página HTML]
    G -->|Comando MQTT (RESET_LEDS)| D
    D -->|Callback| B

    G -->|Fetch API /get e /list| E
    E -->|Resposta JSON| G
