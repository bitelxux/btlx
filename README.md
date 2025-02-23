# **BTLX25 Library - User Guide**

## Overview
The **BTLX25** library is designed for ESP8266-based applications, providing WiFi management, logging, timers, and OTA updates.

## Features
- **WiFi management** using WiFiManager
- **Logging** to a server or serial console
- **Timers and Tickers** for periodic tasks
- **NTP time synchronization**
- **EEPROM handling** for persistent storage
- **OTA updates** via ArduinoOTA

---

## **Installation**
1. Install **Arduino IDE** with ESP8266 board support.
2. Include required dependencies:
   ```cpp
   #include <Arduino.h>
   #include <ESP8266WiFi.h>
   #include <ESP8266WebServer.h>
   #include <EEPROM.h>
   #include <ArduinoOTA.h>
   #include <WiFiManager.h>
   ```
3. Add `btlx25.h` and `btlx25.cpp` to your project.

---

## **Usage**
### **Creating an App Instance**
```cpp
App myApp("DeviceID", "http://logserver.com", LED_BUILTIN);
```
- **DeviceID**: Unique identifier for the device.
- **log_server**: (Optional) Server URL for logs.
- **controlLed**: GPIO pin for status LED.

### **Starting WiFi Manager**
```cpp
bool connected = myApp.startWiFiManager();
if (connected) {
    Serial.println("Connected to WiFi!");
}
```

### **Logging Messages**
```cpp
myApp.log("System started!");
```

### **Adding Timers**
Run a function periodically:
```cpp
myApp.addTimer(5000, &App::myFunction, "TimerName");
```
- Runs `myFunction()` every 5 seconds.

### **Controlling LED**
```cpp
myApp.blinkControlLed(500); // Blink LED every 500ms
```

### **Handling OTA Updates**
```cpp
myApp.handleOTA();
```

### **Checking WiFi Connection**
```cpp
myApp.checkConnection();
```

### **Getting Current Time (Epoch)**
```cpp
unsigned long epoch = myApp.getEpochSeconds();
```

### **EEPROM Operations**
```cpp
myApp.incBoots();  // Increment boot counter
myApp.resetBoots(); // Reset boot counter
```

---

## **Example Sketch**
```cpp
#include "btlx25.h"

App myApp("Device123", "http://logserver.com", LED_BUILTIN);

void setup() {
    Serial.begin(115200);
    myApp.startWiFiManager();
    myApp.log("Device started!");
}

void loop() {
    myApp.handleOTA();
    myApp.attendTimers();
}
```

---

## **Troubleshooting**
1. **WiFi not connecting?**  
   - Ensure correct SSID and password.  
   - Call `myApp.resetWIFI();` to reset WiFi settings.
   
2. **OTA update fails?**  
   - Ensure the device is connected to WiFi.  
   - Use `ArduinoOTA.begin();` in `setup()`.

3. **Timers not working?**  
   - Call `myApp.attendTimers();` in `loop()`.

---

This should help you get started with the **BTLX25** library! 🚀

