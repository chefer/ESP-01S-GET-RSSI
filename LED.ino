
static void setLed(int on) {
  int8_t pin = BlUE_LED;
  if (pin < 0) return;

  // on = 1 LED LIGADO
  if (on) {
    //ON - SET GPIO2(pin) COM SAÍDA PARA NÍVEL ALTO 
    gpio_output_set(0, (1 << pin), (1 << pin), 0); 
  } else {
    // OFF - SET GPIO2(pin)
    gpio_output_set((1 << pin), 0, (1 << pin), 0);
  }
}

void blinkTimerCB() {
  static uint8_t LEDState = 0;
  int time = 1000;
  switch (WiFiState) {
    case WiFiGotIP:
      if (MQTTState == MQTTIsDisconnected) {
        //Serial.println(" WiFiGotIP && MQTTIsDisconnected");
        LEDState = 1 - LEDState;
        time = LEDState ? 500 : 2500;
        break;
      }
      //Serial.println("WiFiGotIP");
      LEDState = 1 - LEDState;
      time = LEDState ? 100 : 2900;
      break;
    case WiFiIsConnected:
      if (WiFiState == WiFiDHCPTimeOut) {
        //Serial.println(" WiFiIsConnected && WiFiDHCPTimeOut");
        LEDState = 1 - LEDState;
        time = LEDState ? 1000 : 200;
        break;
      }
      //Serial.println("WiFiIsConnected");
      LEDState = 1 - LEDState;
      time = LEDState ? 100 : 1000;
      break;
    case WiFiIsDisconnected:
      //Serial.println("WiFiIsDisconnected");
      LEDState = 1 - LEDState;
      time = LEDState ? 1000 : 100;
      break;
    case WiFiAUTHMODE:
      //Serial.println("WiFiAUTHMODE");
      LEDState = 1;
      time = LEDState ? 1000 : 1000;
      break;
    default:
      //Serial.println("Status inesperado");
      LEDState = 1;
      time = LEDState ? 1000 : 1000;
      break;
  }
  setLed(LEDState);
  os_timer_arm(&LEDTimer, time, 0);
}
