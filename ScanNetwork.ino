
void scanNetwork() {

  if (!scanAPs.scanInProgress) {
    scanAPs.scanInProgress = SCANNING;

    /*TEMPO DE VARREDURA PASSIVA POR CANAL EM MS,
       MAX: 1000ms E MIN 100ms.*/
    uint32_t scan_time = 100;

    wifi_scan_time_t passive_scan_time;
    passive_scan_time.passive = scan_time;

    struct scan_config config = { NULL, NULL, 0, 0, WIFI_SCAN_TYPE_PASSIVE, passive_scan_time};

    wifi_station_scan(&config, scanNetworkDoneCb);
  }
}

/*ICACHE_FLASH_ATTR, INDICA À FUNÇÃO QUE OS DADOS DEVEM SER ARMAZENADAS NA MEMÓRIA FLASH INVÉS DA DRAM.
  É BASTANTE RECOMENDÁVEL UTILIZAR ICACHE_FLASH_ATTR PARA ECONOMIZAR RAM*/
static void ICACHE_FLASH_ATTR scanNetworkDoneCb(void *bss_struct, STATUS status) {
  int n;
  struct bss_info *bssData = (struct bss_info *)bss_struct;

  if (status != OK) {
    scanAPs.scanInProgress = NOT_SCANNING;
    return;
  }

  //SE FOR NECESSÁRIO LIMPAR OS DADOS DOS APs ANTERIORES
  if (scanAPs.apData != NULL) {
    for (n = 0; n < scanAPs.numAPs; n++) {
      os_free(scanAPs.apData[n]);
    }
    os_free(scanAPs.apData);
  }

  //CONTA OS APs ENCONTRADOS
  n = 0;
  while (bssData != NULL) {
    bssData = bssData->next.stqe_next;
    n++;
  }

  /*A ALOCAÇÃO DE MEMÓRIA DINÂMICA POR os_malloc DEVE SER EVITADA.
     A ALOCAÇÃO E LIBERAÇÃO DE OBEJETOS COM TAMANHOS DIFERENTES LEVA MUITO RAPIDAMENTE
     À FRAGMENTAÇÃO DA LIMITADA RAM NO ESP8266.
     USE ALOCAÇÃO PARA .bss OU buffers EM TEMPO DE COMPILAÇÃO,
     DECLARADOS COMO FUMÇÃO INTERNA ESTÁTICA OU COMO ALOCAÇÃO EM NIVEL GLOBAL.*/

  /*A FUNÇÃO heapMem() EXIBE NO MONITOR SERIAL A QUANDIDADE DE MEMÓRIA LIVRE */

  //ALOCA MEMÓRIA PARA OS PONTEIROS DOS APs
  scanAPs.apData = (AP **)os_malloc(sizeof(AP *)*n);
  scanAPs.numAPs = n;

  //ALOCA OS APs NA ESTRUTURA
  n = 0;
  bssData = (struct bss_info *)bss_struct;

  while (bssData != NULL) {
    if (n >= scanAPs.numAPs) {
      //SE FALTAR MEMÓRIA ALOCADA O PROCESSO PARA!
      break;
    }

    //ALOCA MEMÓRIA PARA O AP n.
    scanAPs.apData[n] = (AP *)os_malloc(sizeof(AP));
    //PASSA O AP PARA A ESTRUTURA scanAPs
    strncpy(scanAPs.apData[n]->ssid, (char*)bssData->ssid, 32);
    scanAPs.apData[n]->rssi = bssData->rssi;
    scanAPs.apData[n]->authmode = bssData->authmode;
    strcpy(scanAPs.apData[n]->bssid, bssidToChar(bssData->bssid));
    scanAPs.apData[n]->channel = bssData->channel;
    scanAPs.apData[n]->is_hidden = bssData->is_hidden;;

    bssData = bssData->next.stqe_next;
    n++;
  }

  //IMPRIME AS REDES ESCANEADAS
  printAPs(scanAPs.apData, scanAPs.numAPs);

  //ORDENAÇÃO DECRESECNTE DAS BSSs PELA FORÇA DO SINAL DE RSSI
  apData **apDataPt = scanAPs.apData;
  qsort(apDataPt, scanAPs.numAPs, sizeof(struct apData*), compareRSSI);

  scanAPs.scanInProgress = NOT_SCANNING;
}

int compareRSSI(const void *bss1, const void *bss2) {
  struct apData **firstAP = (struct apData **)bss1;
  struct apData **secondAP = (struct apData **)bss2;
  return ((*secondAP)->rssi - (*firstAP)->rssi);
}

void printAPs(struct apData **arrayAPs, size_t len) {
  size_t i;
  for (i = 0; i < len; i++) {
    printf("[ SSID: %s \t RSSI: %d ]\n", arrayAPs[i]->ssid, arrayAPs[i]->rssi);
  }
  puts("--");
}

char* bssidToChar(uint8 * bssid) {
  char buffer[18];
  snprintf(buffer, sizeof(buffer), "%02x:%02x:%02x:%02x:%02x:%02x",
           bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  return buffer;
}
