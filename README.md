# Arduino_Ethernet_Var_Sync
Um Sincronismo de variavel via Shield de Ethernet
## Funcionalidades

* **Sincronização P2P:** Sem necessidade de servidor central (Master/Slave). Todos os nós são iguais.
* **Arbitragem por Timestamp:** Conflitos de dados resolvidos via comparação de Epoch Time (o dado mais recente sempre vence).
* **Persistência em SD:** Estado das variáveis preservado após queda de energia ou reboot.
* **Protocolo NTP:** Sincronização de relógio global via rede para garantir precisão entre múltiplos Arduinos.
* **Eficiência de Memória:** Otimizado para os 2KB de RAM do ATmega328P usando estruturas binárias empacotadas.

## Hardware Necessário

* Arduino Uno R3 (ou compatível).
* Ethernet Shield W5100 (com slot para MicroSD).
* Cartão MicroSD (Formatado em FAT16 ou FAT32).
* Cabo de Rede RJ45.

## Configuração de Pinos (Padrão Shield)

| Componente | Pino (CS) | Protocolo |
| :--- | :--- | :--- |
| **Ethernet W5100** | 10 | SPI |
| **SD Card** | 4 | SPI |
| **SPI (Shared)** | 11, 12, 13 | MOSI, MISO, SCK |

## Estrutura de Dados Escalável

Para adicionar novas variáveis ao sistema, basta expandir a `struct` no código fonte. O sistema calcula automaticamente o tamanho do pacote para validação.

```cpp
struct __attribute__((packed)) SyncData {
  uint32_t ts_var1; // Timestamp (4 bytes)
  int16_t var1;     // Dado 1 (2 bytes)
  uint32_t ts_var2; // Timestamp (4 bytes)
  float var2;       // Dado 2 (4 bytes)
  // Adicione novos campos mantendo o par [timestamp + variável]
};
