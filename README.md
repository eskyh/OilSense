## Home Cloud-based Smart Heating Oil Monitoring and Ordering Decision System

## 1. System Introduction

This project developed a smart IoT system that combines a **Raspberry Pi Zero** as the central home cloud server (Running **Node-RED** and **MQTT** broker) with distributed smart **ultrasonic sensors** (based on the **D1 Mini-ESP8266**) to monitor home heating oil levels. The system sends alerts to your phone when oil levels drop below a set threshold and features **one-button oil ordering** from the best deal in the marketplace. This star architecture can be easily extended to other smart home applications, such as sump pit water level monitoring, as shown in Figure 1.

    

                                            **Figure 1. System architecture**   

<img src="doc/System_Diagram.svg" title="" alt="System Architecture" data-align="center">

## 2. Source code structure

[**`/gauge/src/`**](https://github.com/eskyh/OilSense/tree/main/gauge/src)

[`main.cpp`](https://github.com/eskyh/OilSense/tree/main/gauge/src/main.cpp): The main function of **D1 Mini (ESP8266)** microcontroller firmware. It calls `EspClient` class to do the following works:

`setup()` : Executed once at the start of the program, this phase is typically used for configuration and hardware initialization. Invoke `EspClient::setup()` to load configurations, establish WiFi/MQTT/OTA connections, initialize sensors, and sync time with the NTP server.

`loop()` : Executed repeatedly (hence the name 'loop'). Invoke `EspClient::loop()` to do sensor measurements, MQTT subscribing/publishing messages, processing MQTT actuator commands, managing the web portal, uploading firmware via WiFi, and restarting the device.

    

[**`/myLibs/network/`**](https://github.com/eskyh/OilSense/tree/main/myLibs/network)

[`EspClient.hpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/network/EspClient.hpp), [`EspClient.cpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/network/EspClient.cpp): The primary class responsible for managing all functions in the firmware and is designed as a singleton.

[`JTimer.h`](https://github.com/eskyh/OilSense/tree/main/myLibs/network/JTimer.h), [`JTimer.cpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/network/JTimer.cpp): Designed to provide an efficient timing mechanism within the firmware. It simplifies the management of timed events and callbacks and is designed as a singleton.

[`Config.hpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/network/Config.hpp), [`Config.cpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/network/Config.cpp) : Managing configuration settings in a JSON format.

<img src="doc/EspClient.svg" title="" alt="EspClient class diagram" data-align="center">

<img src="doc/JTimer.svg" title="" alt="JTimer class diagram" data-align="center">

    

[**`/myLibs/sensors/`**](https://github.com/eskyh/OilSense/tree/main/myLibs/sensors)

[`sensor.hpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/sensors/sensor.hpp), [`sensor.cpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/sensors/sensor.cpp): The base **Sensor** calss for other derived sensors below:

- [`sr04.hpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/sensors/sr04.hpp), [`sr04.cpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/sensors/sr04.cpp): Ultrasonic range sensor class for the HC-SR04.

- [`dht11.hpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/sensors/dht11.hpp), [`dht11.cpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/sensors/dht11.cpp): Temperature and humidity sensor class for DHT11.

- [`vl53l0x.hpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/sensors/vl53l0x.hpp), [`vl53l0x.cpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/sensors/vl53l0x.cpp): Infrared distance sensor class for VL53L0X.

    

[`filter.hpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/sensors/filter.hpp), [`filter.cpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/sensors/filter.cpp): Data filtering algorithm classes: **Median**, **Kalman**, **EWMA**.

[`band.hpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/sensors/band.hpp), [`band.cpp`](https://github.com/eskyh/OilSense/tree/main/myLibs/sensors/band.cpp):  Band filter class supporting both dead band and narrow band.

    

The **`Sensor`** base class encapsulates common functionalities shared across all sensor types, such as:

* `_read()`: A pure virtual function that each derived sensor class implements to handle sensor-specific reading logic.  
* `setFilter()`: Allows the assignment of a filter (e.g., Median, Kalman, EWMA) to process sensor data.  
* `setMqtt()`: Set MQTT client for data transmission.  
* `getPayload()`: A pure virtual function that is implemented by each derived class to format the sensor's data for transmission.  
* `sendMeasure()`: A method that handles data communication or publication to an MQTT broker or other destinations.

<img src="doc/Sensor.svg" title="" alt="Sensor class diagram" data-align="center">

[**`/gauge/web/`**](https://github.com/eskyh/OilSense/tree/main/gauge/web)

[`main.html`](https://github.com/eskyh/OilSense/tree/main/gauge/web/main.html) : A lightweight, all-in-one web portal developed for smart sensor management on D1 Mini. Users can configure settings and perform firmware upgrades over Wi-Fi. This file relies on the following JavaScript and CSS code.

[`espman.js`](https://github.com/eskyh/OilSense/tree/main/gauge/web/espman.js) :  JavaScript code provides the following functions for the web portal: AJAX-based retrieving/updating smart sensor configuration using JSON, adding/removing sensors, pin configurations, file management, firmware uploads, and device restarts.

[`filedrag.js`](https://github.com/eskyh/OilSense/tree/main/gauge/web/filedrag.js) :JavaScript code enabling drag-and-drop functionality on the *File Management* page of the web portal.

[`style.css`](https://github.com/eskyh/OilSense/tree/main/gauge/web/style.css) : The stylesheet for the web portal.

[`release.bat`](https://github.com/eskyh/OilSense/tree/main/gauge/web/release.bat) : A batch command file compresses the `main.html` and all supporting script files into a single HTML, then gzips it. This significantly improves web portal access performance.

[`/config/`](https://github.com/eskyh/OilSense/tree/main/gauge/web/config) :  This subfolder contains three sample configuration JSON files.

                                    **Figure 2. Web portal for Microcontroller**

<img src="doc/Web_Portal.svg" title="" alt="Web portal for smart sensor" data-align="center">

    

[**`/tools/`**](https://github.com/eskyh/OilSense/tree/main/tools)

[`index.html`](https://github.com/eskyh/OilSense/tree/main/tools/index.html): A web-based tool I developed to display multi-time series data (e.g., oil level data from `data.series.csv`) or marketplace quotes from various dealers (e.g., `data.quotes.csv` as shown below).

    

<img src="doc/quotes.png" title="" alt="Marketplace quotes chart" data-align="center">

    

[**`/gauge/3d_model/`**](https://github.com/eskyh/OilSense/tree/main/gauge/3d_model)

A series of 3D model of internal structural components designed to support the D1 Mini and connected sensors. `model_final_subtract.3mf` is the final design, while the others are either intermediate versions or older designs where the sensor seat location caused interference with the housing.

<img title="" src="doc/3D-Print.svg" alt="3D design model" data-align="center">

## 3. Node-RED Workflow and Dashboard

The Node-RED data flows (Not included in this repository) consist of two components outlined below. They are rendered as dashboards that are accessible on your phone from anywhere (**NOTE**: Port forwarding needs to be configured in the home router!).

1. Oil Level Monitoring: This includes oil level monitoring, data management, and dashboard functionalities. Check out the dashboard demonstration in Figure 3 and the simplified version of the actual worflow behind the scene in Figure 5 and 6.

2. Oil Marketplace and Ordering: This covers the oil price marketplace and ordering functionalities. See the dashboard demonstration in Figure 4.

                           **Figure 3. Dashboard of the oil monitoring system.**

<img src="file:///D:/maker_projects/doc/OilGauge.svg" title="" alt="OilGauge.svg" data-align="center">

                                    **Figure 4. Oil Marketplace Dashboard**

<img src="file:///D:/maker_projects/doc/marketplace.svg" title="" alt="marketplace.svg" data-align="center">

                    **Figure 5. Subscribed sensor data processing workflow**

![flow-oilgauge.svg](D:\maker_projects\doc\flow-oilgauge.svg)

          

                                            **Figure 6. Actuator workflow**

![flow-actuator.svg](D:\maker_projects\doc\flow-actuator.svg)
