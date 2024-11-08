## Home Cloud-based Smart Heating Oil Monitoring and Ordering Decision System

This project developed a smart IoT system that combines a **Raspberry Pi Zero** as the central home cloud server (Running **Node-RED** and **MQTT** broker) with distributed smart **ultrasonic sensors** (based on the **D1 Mini-ESP8266**) to monitor home heating oil levels. The system sends alerts to your phone when oil levels drop below a set threshold and features **one-button oil ordering** from the best available deal in the marketplace. This star architecture can be easily extended to other smart home applications, such as sump pit water level monitoring, as shown in Figure 1.

    

                                            **Figure 1. System architecture**   

<img src="doc/System_Diagram.svg" title="" alt="System Architecture" data-align="center">

## 1. Dashboard and Node-RED Workflow

The Node-RED data flows consist of two components outlined below. They are rendered as dashboards that are accessible on your phone from anywhere (**NOTE**: Port forwarding needs to be configured in the home router!).

1. **Oil Level Monitoring**: This includes oil level monitoring, data management, and dashboard functionalities. Check out the dashboard demonstration in Figure 2 and the simplified worflow (for convenience of demonstration) behind the scene in Figure 4 and 5.

2. **Oil Marketplace and Ordering**: This covers the oil price marketplace and ordering functionalities. See the dashboard demonstration in Figure 3 and the simplified workflow behind the scene in Figure 6.

                           **Figure 2. Oil level monitoring dashboard.**

<img src="doc/OilGauge.svg" title="" alt="Oil level monitoring dashboard" data-align="center">

                                    **Figure 3. Oil Marketplace and ordering dashboard**

<img title="" src="doc/marketplace.svg" alt="Heating oil marketplace and ordering dashboard" data-align="center" width="540">

                    **Figure 4. Sensor data processing workflow in Node-RED**

<img src="doc/flow-oilgauge.svg" title="" alt="Sensor data processing workflow in Node-RED" data-align="center">

`1. Receive sensor measurement data messages from the subscribed MQTT broker.`
`2. Apply filtering algorithms (Median, Kalman Filter, and EWMA) to the measurements.`
`3. Calculate the heating oil level in the tank based on the distance measured from the top of the tank to the oil surface.`
`4. Only pass messages if the change in measurement exceeds a specified threshold.`
`5. Display the oil level and historical data, including daily consumption, on the dashboard.`
`6. Save oil level data to a file.`
`7. Trigger an alarm and send a notification to mobile devices via Telegram bot.`
`8. Toggle notifications on or off via Telegram messages or from the dashboard on mobile.`
`9. Display diagnostic information and environmental data (temperature and humidity) on the dashboard.`

    

                                            **Figure 5. Actuator workflow in Node-RED**
<img src="doc/flow-actuator.svg" title="" alt="Actuator workflow in Node-RED" data-align="center">

`1. Receive heartbeat, commands, and information messages from the subscribed MQTT broker.`
`2. Display configuration controls for MCU devices (e.g., toggle LED on/off, enable/disable automatic measurement, set measurement intervals, select data filters, enable/disable sensors, trigger a one-time measurement, restart the smart sensor, etc.`
`3. Publish configuration options as retained messages to the MQTT broker, so all subscribed smart sensors will automatically configure themselves based on these settings.`
`4. Publish acturator MQTT messages, so all subscribed smart sensors will execute the corresponding actions.`

    

                                            **Figure 6. Marketplace workflow in Node-RED**
<img title="" src="doc/flow-marketplace.svg" alt="Marketplace workflow in Node-RED" data-align="center" width="714">

`1. Collect the real-time futures data.`
`2. Collect real-time local marketplace quotes.`
`3. Dashboard controls and charts showing prices.`
`4. Same as 2 but for another city.`
`5. Set order type (cash/credit) and order amount.`
`6. Construct order with preset form fields and placing order.`

## 2. Source Code Structure

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

                                    **Figure 7. Web portal for Microcontroller**

<img src="doc/Web_Portal.svg" title="" alt="Web portal for smart sensor" data-align="center">

    

[**`/tools/`**](https://github.com/eskyh/OilSense/tree/main/tools)

[`index.html`](https://github.com/eskyh/OilSense/tree/main/tools/index.html): A web-based tool I developed to display multi-time series data (e.g., oil level data from `data.series.csv`) or marketplace quotes from various dealers (e.g., `data.quotes.csv` as shown below).

    

<img src="doc/quotes.png" title="" alt="Marketplace quotes chart" data-align="center">

    

[**`/gauge/3d_model/`**](https://github.com/eskyh/OilSense/tree/main/gauge/3d_model)

A series of 3D model of internal structural components designed to support the D1 Mini and connected sensors. `model_final_subtract.3mf` is the final design, while the others are either intermediate versions or older designs where the sensor seat location caused interference with the housing.

<img title="" src="doc/3D-Print.svg" alt="3D design model" data-align="center">
