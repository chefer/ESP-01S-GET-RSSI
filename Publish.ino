
void transmitJsonRSSIData(DynamicJsonDocument data) {
  char buffer[23];
  snprintf(buffer, sizeof(buffer), "/ESP-01/%s/JSON_RSSI/", DEVICE_LABEL);

  String sData = "";
  serializeJson(data, sData);

  MQTTClient.publish(buffer, String(sData).c_str(), false);
}
void pubRSSI(sint8 rssi) {
  char buffer[19];
  snprintf(buffer, sizeof(buffer), "/ESP-01/%s/RSSI/", DEVICE_LABEL);

  MQTTClient.publish(buffer, String(rssi).c_str(), false);
}

void pubQuality(uint8_t quality) {
  char buffer[21];
  snprintf(buffer, sizeof(buffer), "/ESP-01/%s/quality/", DEVICE_LABEL);

  MQTTClient.publish(buffer, String(quality).c_str(), false);
}

void pubVCC(float voltage) {
  char buffer[21];
  snprintf(buffer, sizeof(buffer), "/ESP-01/%s/voltage/", DEVICE_LABEL);

  MQTTClient.publish(buffer, String(voltage).c_str(), false);
}

void transmitJsonScannedNetwork(DynamicJsonDocument data) {
  char buffer[31];
  snprintf(buffer, sizeof(buffer), "/ESP-01/%s/JSON_SCAN_NETWORK/", DEVICE_LABEL);

  String sData = "";
  serializeJson(data, sData);
  const size_t capacity = sData.length();
  MQTTClient.beginPublish(buffer, capacity, false);
  MQTTClient.print(String(sData).c_str());
  MQTTClient.endPublish();
}
