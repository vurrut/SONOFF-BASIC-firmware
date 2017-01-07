/*
 Sonoff: firmware
 More info: https://github.com/tschaban/SONOFF-firmware
 LICENCE: http://opensource.org/licenses/MIT
 2016-10-27 tschaban https://github.com/tschaban
*/

void runSwitchMode() {
  LED.On();
  Serial << endl << "Device mode: SWITCH" << endl;
  Serial << endl << "Configuring MQTT" << endl;
  client.setServer(sonoffConfig.mqtt_host, sonoffConfig.mqtt_port);
  client.setCallback(callbackMQTT);
  connectToWiFi();

  if (sonoffConfig.temp_present) {
    Serial << endl << "Starting DS18B20" << endl;
    DS18B20.begin(); 
    setSensorReadInterval(sonoffConfig.temp_interval);
  } else {
    Serial << endl << "DS18B20 not present" << endl; 
  }
}

void runConfigurationLANMode() {
  LED.On();
  Serial << endl << "Device mode: LAN Configuration" << endl;
  WiFi.mode(WIFI_STA);
  connectToWiFi();
  startHttpServer();
  Serial << endl << " - Ready for configuration. Open http://" << WiFi.localIP() << endl << endl;
  LED.startBlinking(0.1);
}

void runConfigurationAPMode() {
  LED.On();
  Serial << endl << "Device mode: Access Point Configuration" << endl;
  Serial << " - launching access point" << endl;
  IPAddress apIP(192, 168, 5, 1);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(sonoffConfig.host_name);
  dnsServer.setTTL(300);
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  dnsServer.start(53, "www.example.com", apIP);
  startHttpServer();
  Serial << " - After conecting to WiFi: " << sonoffConfig.host_name << " open: http://192.168.5.1/  " << endl << endl;
  LED.startBlinking(0.1);  
}

void toggleMode() {
  if (sonoffConfig.mode == 0) {
    memory.saveSwitchMode(1);
  } else {
    memory.saveSwitchMode(0);
  }
  Serial << "Rebooting device" << endl;
  LED.blink();
  delay(1000);
  ESP.restart();
}

void configuratonAPMode() {
  LED.On();
  memory.saveSwitchMode(2);
  Serial << "Rebooting device to Access Point" << endl;
  delay(1000);
  ESP.restart();
}

void resetDeviceMode() {
  LED.On();
  Serial << "- ereasing EEPROM" << endl;
  memory.erase();
  delay(1000);
  ESP.restart();
}
