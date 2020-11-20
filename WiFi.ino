
void enableWiFi() {

  if (WiFiState != WiFiGotIP) {

    wifi_set_opmode_current(STATION_MODE);
    struct station_config sc;
    bzero(&sc, sizeof(struct station_config));
    os_strncpy((char*)sc.ssid, ESSID, 32);
    os_strncpy((char*)sc.password, PASSWD, 64);

    sc.bssid_set = 0;
    sc.threshold.rssi = 0;
    sc.threshold.authmode = AUTH_OPEN;

    wifi_station_set_config(&sc);
    wifi_station_dhcpc_start();
    wifi_station_connect(); //ou wifi_station_set_config_current();
  }

  wifi_set_event_handler_cb(WiFiHandleCB);
}

void WiFiHandleCB(System_Event_t *evt) {

  switch (evt->event) {
    case EVENT_STAMODE_CONNECTED:
      statusWifiUpdate(WiFiIsConnected);
      Serial.println("EVENT_STAMODE_CONNECTED: ");
      Serial.printf("connect to ssid %s, channel %d\n",
                    evt->event_info.connected.ssid,
                    evt->event_info.connected.channel);
      break;
    case EVENT_STAMODE_DISCONNECTED:
      statusWifiUpdate(WiFiIsDisconnected);
      Serial.println("EVENT_STAMODE_DISCONNECTED: ");
      Serial.printf("disconnect from ssid %s, reason %d\n",
                    evt->event_info.disconnected.ssid,
                    evt->event_info.disconnected.reason);
      wifi_station_connect();
      break;
    case EVENT_STAMODE_AUTHMODE_CHANGE:
      statusWifiUpdate(WiFiAUTHMODE);
      Serial.println("EVENT_STAMODE_AUTHMODE_CHANGE: ");
      Serial.printf("mode: %d -> %d\n",
                    evt->event_info.auth_change.old_mode,
                    evt->event_info.auth_change.new_mode);
      break;
    case EVENT_STAMODE_GOT_IP:
      statusWifiUpdate(WiFiGotIP);
      Serial.println("EVENT_STAMODE_GOT_IP: ");
      Serial.printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
                    IP2STR(&evt->event_info.got_ip.ip),
                    IP2STR(&evt->event_info.got_ip.mask),
                    IP2STR(&evt->event_info.got_ip.gw));
      Serial.printf("\n");
      break;
    case EVENT_STAMODE_DHCP_TIMEOUT:
      statusWifiUpdate(WiFiDHCPTimeOut);
      wifi_station_disconnect();
      wifi_station_connect();
      Serial.println("EVENT_STAMODE_DHCP_TIMEOUT: ");
      break;
    default:
      break;
  }
}

void statusWifiUpdate(uint8_t state) {
  WiFiState = state;

  os_timer_disarm(&LEDTimer);
  os_timer_setfn(&LEDTimer, (os_timer_func_t *)blinkTimerCB, (void *)0);
  os_timer_arm(&LEDTimer, 500, 0);
}
