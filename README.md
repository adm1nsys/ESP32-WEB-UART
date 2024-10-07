# ESP32-WEB-UART
ESP32-WEB-UART (WROOM-32)

<h2>USE:</h2>
<b>Connect:</b><br>
RX --> TX <br>
TX --> RX <br>
GND --> GND <br>

<br>
<b>Pinout in Code:</b><br>

```C++
// Setting up UART1
HardwareSerial MySerial(1);  // Use UART1
int RX_PIN = 16;  // RX pin for UART1
int TX_PIN = 17;  // TX pin for UART1
```

<br>
<b>Setting Up Wi-Fi:</b><br>

```C++
// Setting up Wi-Fi
const char* ssid = "SSID";
const char* password = "PASSWORD";
WebServer server(80);
```

<br>
<b>Go by ip in Serial Monitor::</b><br>

```C++
Connecting to WiFi...
Connected to WiFi
192.168.0.39
HTTP server started
```
