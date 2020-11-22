void getVCC() {

  float voltage;
  char buffer[6];
  voltage = system_get_vdd33() / 1024.00f;
  snprintf(buffer, sizeof(6), "%3.1f", voltage);
  module.voltage = voltage;
  //Serial.printf("\nVOLTAGEM : %s\n", buffer);

  // PUBLICA A VOLTAGEM
  pubVCC(voltage);

  /*COLOCA A ESTAÇÃO EM MODO DE SONO PROFUNDO COM CONSUMO DE 20uA
  * NOS TESTES O MÓDULO ESP-01S FUNCIONA COM O MINÍMO DE 1.8V*/
  if (module.voltage < SLEEP_THRESHOLD)
    ESP.deepSleep (ESP.deepSleepMax());
}
