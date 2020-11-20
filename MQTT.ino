
void enableMQTT() {

  if (!MQTTClient.connected()) {
    statusMQTTUpdate(MQTTIsDisconnected);
  }
  if (WiFiState == WiFiGotIP && !MQTTState == MQTTIsConnected) {
    Serial.print("Conectando MQTT... ");
    if (MQTTClient.connect(module.label)) {
      Serial.println("conectado");
      statusMQTTUpdate(MQTTIsConnected);
      getVCC();
    } else {
      statusMQTTUpdate(MQTTIsDisconnected);
      Serial.print("falha, rc=");
      Serial.print(MQTTClient.state());
      Serial.println(" reconectando em 1s");
    }
  }
  os_timer_disarm(&MQTTTimer);
  os_timer_setfn(&MQTTTimer, (os_timer_func_t *)MQTTTimerCB, (void *)0);
  os_timer_arm(&MQTTTimer, 2000, 1);
}

void statusMQTTUpdate(uint8_t state) {
  MQTTState = state;

  if (MQTTState == MQTTIsConnected) {
    ModuleState = ModuleIsConnected;
  } else {
    ModuleState = ModuleIsDisconnected;
  }
}

void MQTTTimerCB() {

  if (!MQTTClient.connected()) {
    statusMQTTUpdate(MQTTIsDisconnected);
  } else {
    statusMQTTUpdate(MQTTIsConnected);
  }
}
