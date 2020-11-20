
DynamicJsonDocument getRSSIData() {
  typedef struct APs {
    char ssid[32];
    sint8 rssi;
    uint8_t quality;
  } AP;
  AP apLog;

  strcpy(apLog.ssid, (char*)WiFi.SSID().c_str());
  apLog.rssi = wifi_station_get_rssi();

  int qualityTemp = constrain(apLog.rssi, -100, -50);
  apLog.quality = map(qualityTemp, -100, -50, 0, 100);

  const size_t capacity = JSON_OBJECT_SIZE(3) + 100;
  DynamicJsonDocument doc(capacity);
  doc["ssid"] = apLog.ssid;
  doc["rssi"] = apLog.rssi;
  doc["quality"] = apLog.quality;

  //serializeJson(doc, Serial);
  pubRSSI(apLog.rssi);
  pubQuality(apLog.quality);

  return doc;
}
