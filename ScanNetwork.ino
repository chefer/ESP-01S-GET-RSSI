
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

  //ALOCA MEMÓRIA PARA OS APs
  scanAPs.apData = (AP **)os_malloc(sizeof(AP *)*n);
  scanAPs.numAPs = n;

  //ALOCA OS APs NA ESTRUTURA
  n = 0;
  bssData = (struct bss_info *)bss_struct;
  while (bssData != NULL) {
    if (n >= scanAPs.numAPs) {
      //SE FALTAR MEMÓRIA O PROCESSO PARA!
      break;
    }
    //Dados do AP salvo.
    /*A alocação de memória dinâmica por os_malloc não deve ser usada. A alocação e liberação de objetos de tamanhos
      diferentes leva rapidamente à fragmentação de uma quantidade limitada de RAM no esp8266.
      Use alocações .bss e buffers em tempo de compilação, declarados como função interna estática ou como alocação em nível global.*/
    scanAPs.apData[n] = (AP *)os_malloc(sizeof(AP));
    strncpy(scanAPs.apData[n]->ssid, (char*)bssData->ssid, 32);
    scanAPs.apData[n]->rssi = bssData->rssi;
    scanAPs.apData[n]->authmode = bssData->authmode;
    strcpy(scanAPs.apData[n]->bssid, bssidToChar(bssData->bssid));
    scanAPs.apData[n]->channel = bssData->channel;
    scanAPs.apData[n]->is_hidden = bssData->is_hidden;;

    bssData = bssData->next.stqe_next;
    n++;
  }

  //ORDENAÇÃO DECRESECNTE DAS BSSs PELA FORÇA DO SINAL DE RSSI
  // compareRSSI() PRECISA DE ALTERAÇÃO
  //qsort(scanAPs.apData, scanAPs.numAPs, sizeof(struct AP), compareRSSI);

  scanAPs.scanInProgress = NOT_SCANNING;
}

int compareRSSI(const void *bss1, const void *bss2) {
  int first = ((struct AP *)bss1)->rssi;
  int second = ((struct AP *)bss2)->rssi;
  return (second - first);
}

char* bssidToChar(uint8 * bssid) {
  char buffer[18];
  snprintf(buffer, sizeof(buffer), "%02x:%02x:%02x:%02x:%02x:%02x",
           bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  return buffer;
}
