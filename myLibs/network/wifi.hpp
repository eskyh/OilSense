#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include <ArduinoOTA.h>

#include <Ticker.h>

void setupWifi();
void connectToWifi();
void onWifiConnect(const WiFiEventStationModeGotIP& event);
void onWifiDisconnect(const WiFiEventStationModeDisconnected& event);
void setUpOTA();
