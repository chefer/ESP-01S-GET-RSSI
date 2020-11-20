/////////////////////////////////////////////////////////////////
//         ESP8266 RSSI Signal Station Project     v1.00       //
//         Get the latest version of the code here:            //
//         https://github.com/chefer/ESP-01S-GET-RSSI          //
/////////////////////////////////////////////////////////////////

//ESP8266 Arduino Core’s documentation
//https://arduino-esp8266.readthedocs.io/en/latest/index.html

//EXPOEM AS FUNCIONALIDADES DO SDK DA ESPRESSIF NO ARDUINO IDE
#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif

//O ESP8266 POSSUI CERCA DE 80 KiB DE RAM PARA DADOS (DRAM)
// MODIFICA A FUNÇÃO PARA EXECUTAR DIRETAMENTE DA MEMÓRIA FLASH E NÃO DA IRAM
//https://bbs.espressif.com/viewtopic.php?t=1183#p3964
#define ICACHE_FLASH_ATTR

// ALTERA OS PARAMETROS DE REDE E MQTT
#include "secrets.h"
#include "ESP8266WiFi.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ArduinoTrace.h>

WiFiClient wifiClient;
PubSubClient MQTTClient(MQTT_IP, 1883, wifiClient);
//PELO TESTES COM O ESP-01S O RANGE DA VOLTAGEM EM OPERAÇÃO DO ESP8266 ESTA ENTRE 1.8V ~ 3.6V p.12
//https://www.espressif.com/sites/default/files/documentation/2c-esp8266_non_os_sdk_api_reference_en.pdf
// MEDE A VOLTAGEM DE ENTRADA NO PINO 6
ADC_MODE(ADC_VCC);
//OU
//system_adc_read(void);

os_timer_t LEDTimer, MQTTTimer;
os_timer_t WiFiEnableTimer;
static bool MQTTStatusRequest = false;

// --------------- CONSTANTES ---------------

#define LABEL_BUFFER_SIZE (12)

const char * const PHY_MODE[] {
  "",
  "PHY_MODE_11B",
  "PHY_MODE_11G",
  "PHY_MODE_11N"
};

// LIMITE MINÍMO DA VOLTAGEM EM OPERAÇÃO ANTES DE ENTRAR EM MODO SLEEP
// EXISTE UMA OSCILAÇÃO DE 10% DE MARGEM PARA VOLTAGEM REAL
const float SLEEP_THRESHOLD = 2.2;
// INTERVALO DE VERIFICAÇÃO DA CONEXÃO Wi-Fi
const int INTERVAL_WiFi = 3000;
// INTERVALO PARA VERIFICAR SE CONTINUA CONECTADO COM O SERVIDOR MQTT
const int INTERVAL_MQTT = 1000;
// INTERVALO PARA VEFICIAR SE CONTINUA NO ESTADO CONECTADO
const int INTERVAL_MODULE = 500;
// INTERVALO PARA COLETAR A VOLTAGEM
const int INTERVAL_VCC = 60000;
// INTERVALO PARA COLETAR O RSSI DA REDE NA QUAL ESTA CONECTADO
const int INTERVAL_RSSI = 1000;
// INTERVALO PARA ESCANEAR TODAS AS REDES DISPONÍVEIS
const int INTERVAL_SCAN_NETWORK = 10000;
// INTERVALO PARA ENVIAR ARQUIVO JSON AO MQTT COM TODOS OS PARAMETROS DE REDE COLETADOS
const int INTERVAL_TRANSMIT_SCANNED_NETWORK = 13000;

// --------------- VARIÁVEIS ---------------
//ESTRATÉGIAS PARA REDUZIR A FRAGMENTAÇÃO DE MEMÓRIA >> OBJETOS COM VIDA ÚTIL CURTA
//https://cpp4arduino.com/2018/11/06/what-is-heap-fragmentation.html

unsigned long previousMillisWiFi = 0;
unsigned long previousMillisMQTT = 0;
unsigned long previousMillisModule = 0;
unsigned long previousMillisVCC = 0;
unsigned long previousMillisRSSI = 0;
unsigned long previousMillisScanNetwork = 0;
unsigned long previousMillisTransmitScannedNetwork = 0;

// --------------- ESTRUTURAS ---------------
// DADOS DA ESTAÇÃO
struct  {
  char label[LABEL_BUFFER_SIZE];
  char macaddr[18];
  float voltage;
  char connectedToSSID[32];
  sint8 rssi;
  int phyMode;
  IPAddress localIP;
  IPAddress subnetMask;
  IPAddress gatewayIP;
} module;

//Basic Service Set (BSS) ESCANEADO
typedef struct AP {
  char ssid[32];
  sint8 rssi;
  AUTH_MODE authmode;
  char bssid[18];
  int32_t channel;
  uint8 is_hidden;
} AP;
AP apData;

#define SCANNING true
#define NOT_SCANNING false

//LISTA DE TODAS OS BSSs SCANEADOS
typedef struct Scan{
  bool scanInProgress = NOT_SCANNING;
  AP **apData;
  int numAPs;
} Scan;
static Scan scanAPs;

// --------------- CONSTANTES ENUMERADAS ---------------
enum {WiFiIsDisconnected,
      WiFiAUTHMODE,
      WiFiIsConnected,
      WiFiGotIP,
      WiFiDHCPTimeOut
     };
static uint8_t WiFiState = WiFiIsDisconnected;

enum {MQTTIsDisconnected,
      MQTTIsConnected
     };
int MQTTState = MQTTIsDisconnected;

enum {ModuleIsDisconnected,
      ModuleIsConnected,
      ModuleIsSleeping
     };
int ModuleState = ModuleIsDisconnected;


// --------------- PRINCIPAL FUNÇÃO QUANDO A SKETCH INICIA---------------
void setup() {
  Serial.begin(115200);
  // APÓS INICIALIZAR DÊ TEMPO SUFICIENTE PARA ABRIR O MONITOR SERIAL
  os_delay_us(1000); //OU //delay(1000);

  //EXIBE NO MONITOR SERIAL A QUANDIDADE DE MEMÓRIA LIVRE
  heapMem();

  Serial.println("\nINICIALIZANDO...");
  Serial.printf("\nVersão do SDK: %s\n", system_get_sdk_version());

  Serial.print(F("RAM(heap) livre [bytes]: "));
  Serial.println(system_get_free_heap_size());

  Serial.print(F("Frequência da CPU [Mhz]:"));
  Serial.println(system_get_cpu_freq());

  snprintf (module.label, LABEL_BUFFER_SIZE, "/ESP-01/%s/", DEVICE_LABEL); //wifi_station_get_hostname()
  wifi_station_set_hostname(module.label);

  getVCC();
  setLed(1);
  enableWiFi();
  Serial.print("\n\n");
}

void heapMem() {
  //Serial.printf_P(PSTR("Heap livre: %d\n"), ESP.getFreeHeap());
  Serial.printf_P(PSTR("Heap livre: %d\n"), system_get_free_heap_size());
}

// FUNÇÃO QUE REPETE-SE ENQUANTO A ESTAÇÃO ESTIVER LIGADA
void loop() {

  unsigned long currentMillisModule = millis();
  if ((unsigned long)(currentMillisModule - previousMillisModule) >= INTERVAL_MODULE) {
    previousMillisModule = currentMillisModule;
    unsigned long currentMillis = millis();

    switch (ModuleState) {
      case ModuleIsDisconnected:
        if ((unsigned long)(currentMillis - previousMillisWiFi) >= INTERVAL_WiFi) {
          previousMillisWiFi = currentMillis;
          //CONFIGURA O MÓDULO COMO UMA STATION E TENTA CONECTAR AO Wi-FI
          enableWiFi();
          //USADO QUANDO UM FUNÇÃO LEVÃO ALGUM TEMPO PARA SER CONCLUÍDA O CONTROLE É PASSADO PARA OUTRAS TAREFAS.
          yield();

          // PEGA AS INFORMAÇÕES DO MÓDULO
          strcpy(module.macaddr, (char*)WiFi.macAddress().c_str());
          strcpy(module.connectedToSSID, (char*)WiFi.SSID().c_str());
          module.phyMode = wifi_get_phy_mode();
          module.localIP = WiFi.localIP();
          module.subnetMask = WiFi.subnetMask();
          module.gatewayIP = WiFi.gatewayIP();
        }
        // FLAG PARA NAO EXECUTAR MQTT ANTES DO Wi-Fi...
        if ((unsigned long)(currentMillis - previousMillisMQTT) >= INTERVAL_MQTT) {
          previousMillisMQTT = currentMillis;
          //CONECTA O MÓDULO AO MQTT
          enableMQTT();
        }

        Serial.println("-----ModuleIsDisconnected------");
        Serial.printf("WiFiState %d\n", WiFiState);
        Serial.printf("MQTTState %d\n", MQTTState);
        break;
      case ModuleIsConnected:

        if ((unsigned long)(currentMillis - previousMillisVCC) >= INTERVAL_VCC) {
          previousMillisVCC = currentMillis;
          getVCC();
        }
        if ((unsigned long)(currentMillis - previousMillisRSSI) >= INTERVAL_RSSI) {
          previousMillisRSSI = currentMillis;
          transmitJsonRSSIData(getRSSIData());
        }
        if ((unsigned long)(currentMillis - previousMillisScanNetwork) >= INTERVAL_SCAN_NETWORK) {
          previousMillisScanNetwork = currentMillis;
          heapMem();
          scanNetwork();
        }
        if ((unsigned long)(currentMillis - previousMillisTransmitScannedNetwork) >= INTERVAL_TRANSMIT_SCANNED_NETWORK) {
          previousMillisTransmitScannedNetwork = currentMillis;
          if (!scanAPs.scanInProgress) {
            transmitJsonScannedNetwork(jsonNetwork());
          }
        }
        //Serial.println("-------ModuleIsConnected-------");
        break;

      default:
        break;
    }
    //MANTENDO VIVA A CONEXÃO COM MQTT
    if (MQTTClient.connected()) {
      MQTTClient.loop();
    } else {
      enableMQTT();
    }
  }

}
