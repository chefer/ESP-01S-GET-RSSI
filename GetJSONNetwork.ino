DynamicJsonDocument jsonNetwork() {

  const size_t capacity = JSON_ARRAY_SIZE(scanAPs.numAPs) + scanAPs.numAPs * JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(10) + 800;

  DynamicJsonDocument doc(capacity);
  doc["label"] = module.label;
  doc["mac"] = module.macaddr;
  doc["voltage"] = String(module.voltage);
  doc["ssid"] = String(module.connectedToSSID);
  doc["rssi"] = String(wifi_station_get_rssi()); //String(module.rssi);
  doc["phy_mode"] = PHY_MODE[module.phyMode];
  doc["localIP"] = module.localIP.toString();
  doc["subnetMask"] = module.subnetMask.toString();
  doc["gatewayIP"] = module.gatewayIP.toString();

  JsonArray rede = doc.createNestedArray("rede");
  for (int i = 0; i < scanAPs.numAPs; i++) {
    JsonObject bss = rede.createNestedObject();
    bss["ssid"] = scanAPs.apData[i]->ssid;
    bss["rssi"] = scanAPs.apData[i]->rssi;
    bss["authmode"] = authModeSTR(scanAPs.apData[i]->authmode);
    bss["bssid"] = scanAPs.apData[i]->bssid;
    bss["channel"] = scanAPs.apData[i]->channel;
    bss["is_hidden"] = scanAPs.apData[i]->is_hidden;
  }

  return doc;
}

String authModeSTR(uint8_t auth) {
  switch (auth) {
    case AUTH_OPEN:
      return "OPEN";
    case AUTH_WEP:
      return "WEP";
    case AUTH_WPA_PSK:
      return "WPA_PSK";
    case AUTH_WPA2_PSK:
      return "WPA2_PSK";
    case AUTH_WPA_WPA2_PSK:
      return "WPA_WPA2_PSK";
    default:
      return "UNKNOWN";
  }
}
