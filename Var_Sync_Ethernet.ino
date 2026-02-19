#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SD.h>

const int PIN_SD_CS = 4;
const int PIN_ETH_CS = 10;

// Configuração de Rede (Alterar MAC e IP para cada placa na rede)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x02 };
IPAddress ip(192, 168, 1, 102);
IPAddress broadcast(255, 255, 255, 255);
const unsigned int localPort = 8888;
const unsigned int ntpPort = 8888;

EthernetUDP Udp;

// Servidor NTP (A.ROOT-SERVERS.NET / pool.ntp.org)
IPAddress timeServer(129, 6, 15, 28);
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

uint32_t lastEpochNTP = 0;
uint32_t millisAtLastNTP = 0;

// Estrutura empacotada e persistente
struct __attribute__((packed)) SyncData {
  uint32_t ts_var1;
  int16_t var1;
  uint32_t ts_var2;
  float var2;
};

SyncData localData = {0, 0, 0, 0.0};
const char* dataFile = "sync.dat";

void setup() {
  pinMode(PIN_SD_CS, OUTPUT);
  pinMode(PIN_ETH_CS, OUTPUT);

  // Inicializa SD
  digitalWrite(PIN_ETH_CS, HIGH);
  SD.begin(PIN_SD_CS);

  // Inicializa Ethernet
  digitalWrite(PIN_SD_CS, HIGH);
  Ethernet.begin(mac, ip);
  Udp.begin(localPort);

  loadFromSD();
  syncNTP();
}

void loop() {
  receiveSync();

  // Re-sincronização NTP a cada 24 horas (86400000 ms)
  if (millis() - millisAtLastNTP > 86400000UL) {
    syncNTP();
  }

  // Gatilho de mutação local (inserir lógica de sensores aqui)
  // Exemplo de atualização: updateLocalVariable1(novo_valor);
}

uint32_t getTimestamp() {
  if (lastEpochNTP == 0) return 0; // NTP não sincronizado
  return lastEpochNTP + ((millis() - millisAtLastNTP) / 1000);
}

void updateLocalVariable1(int16_t newValue) {
  uint32_t currentTS = getTimestamp();
  if (currentTS == 0) return; 

  localData.var1 = newValue;
  localData.ts_var1 = currentTS;
  
  saveToSD();
  broadcastSync();
}

void updateLocalVariable2(float newValue) {
  uint32_t currentTS = getTimestamp();
  if (currentTS == 0) return;

  localData.var2 = newValue;
  localData.ts_var2 = currentTS;
  
  saveToSD();
  broadcastSync();
}

void receiveSync() {
  int packetSize = Udp.parsePacket();
  
  if (packetSize == sizeof(SyncData)) {
    SyncData receivedData;
    Udp.read((char*)&receivedData, sizeof(SyncData));
    bool changed = false;

    // Arbitragem por Timestamp
    if (receivedData.ts_var1 > localData.ts_var1) {
      localData.var1 = receivedData.var1;
      localData.ts_var1 = receivedData.ts_var1;
      changed = true;
    }

    if (receivedData.ts_var2 > localData.ts_var2) {
      localData.var2 = receivedData.var2;
      localData.ts_var2 = receivedData.ts_var2;
      changed = true;
    }

    if (changed) saveToSD();
  }
}

void broadcastSync() {
  Udp.beginPacket(broadcast, localPort);
  Udp.write((uint8_t*)&localData, sizeof(SyncData));
  Udp.endPacket();
}

// Persistência em Cartão SD
void loadFromSD() {
  digitalWrite(PIN_ETH_CS, HIGH);
  digitalWrite(PIN_SD_CS, LOW);

  if (SD.exists(dataFile)) {
    File f = SD.open(dataFile, FILE_READ);
    if (f) {
      f.read((uint8_t*)&localData, sizeof(SyncData));
      f.close();
    }
  }

  digitalWrite(PIN_SD_CS, HIGH);
  digitalWrite(PIN_ETH_CS, LOW);
}

void saveToSD() {
  digitalWrite(PIN_ETH_CS, HIGH);
  digitalWrite(PIN_SD_CS, LOW);

  if (SD.exists(dataFile)) {
    SD.remove(dataFile);
  }
  
  File f = SD.open(dataFile, FILE_WRITE);
  if (f) {
    f.write((uint8_t*)&localData, sizeof(SyncData));
    f.close();
  }

  digitalWrite(PIN_SD_CS, HIGH);
  digitalWrite(PIN_ETH_CS, LOW);
}

// Protocolo NTP
void syncNTP() {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011; 
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  Udp.beginPacket(timeServer, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();

  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    if (Udp.parsePacket()) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      const unsigned long seventyYears = 2208988800UL;
      
      lastEpochNTP = secsSince1900 - seventyYears;
      millisAtLastNTP = millis();
      break;
    }
  }
}