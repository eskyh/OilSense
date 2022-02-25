#pragma once

#include <Arduino.h>

//--------------------------------------------------
#define OTA_HOSTNAME "espSumpPit"
#define OTA_PASSWORD "Hcjmosaic@66"

#ifndef STASSID
	#define STASSID "ESky"
	#define STAPSK  "Hcjmosaic99"
#endif

//--------------------------------------------------
// Raspberri Pi Mosquitto MQTT Broker
//#define MQTT_HOST IPAddress(192, 168, 0, 194)
// For a cloud MQTT broker, type the domain name
#define MQTT_HOST "raspberrypi.local" //Does not work: "esky.tplinkdns.com"
#define MQTT_PORT 1883


//-----------------------------------------------------
#define PRJNAME "sumppit"

#define MQTT_SUB_CMD 	PRJNAME"/cmd/#"

#define CMD_SLEEP       PRJNAME"/cmd/sleep"
#define CMD_LED_BLINK   PRJNAME"/cmd/ledblink"
#define CMD_AUTO        PRJNAME"/cmd/auto"  // auto or manual mode for measurement
#define CMD_MEASURE     PRJNAME"/cmd/measure"
#define CMD_INTERVAL    PRJNAME"/cmd/interval"
#define CMD_RESTART     PRJNAME"/cmd/restart"
#define CMD_SSR_FILTER  PRJNAME"/cmd/filter"

#define CMD_SSR_SR04    PRJNAME"/cmd/sr04"
#define CMD_SSR_VL53    PRJNAME"/cmd/vl53"
#define CMD_SSR_DH11    PRJNAME"/cmd/dh11"

#define MQTT_PUB_SR04   PRJNAME"/sensor/sr04"
#define MQTT_PUB_VL53   PRJNAME"/sensor/vl53"
#define MQTT_PUB_DH11   PRJNAME"/sensor/dh11"

#define MQTT_PUB_HEARTBEAT  PRJNAME"/heartbeat"

#define MQTT_PUB_INFO       PRJNAME"/msg/info"
#define MQTT_PUB_WARN       PRJNAME"/msg/warn"
#define MQTT_PUB_ERROR      PRJNAME"/msg/error"

#define PUB_QOS 0