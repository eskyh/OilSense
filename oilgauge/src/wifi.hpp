#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include <ArduinoOTA.h>

#include <Ticker.h>

#define OTA_HOSTNAME "espOilGauge"
#define OTA_PASSWORD "Hcjmosaic@66"

#ifndef STASSID
	#define STASSID "ESky"
	#define STAPSK  "Hcjmosaic99"
#endif

void setupWifi();
void connectToWifi();
void onWifiConnect(const WiFiEventStationModeGotIP& event);
void onWifiDisconnect(const WiFiEventStationModeDisconnected& event);
void setUpOTA();