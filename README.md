# ESP32-iUART
ESP32-iUART (WROOM-32) <br>
iUART - internet UART<br>
Wireless UART can be used with a web browser on a mobile phone or PC.

<h2>USE:</h2>
<b>Connect:</b><br>
RX --> TX <br>
TX --> RX <br>
GND --> GND <br>
<b>Setup:</b><br>
<ol>
  <li>Add your WIFI to the array.</li>
  <li>Setup static ip in array for every your network.</li>
  <li>Flash the board.</li>
  <li>Go in web-browser http://X.X.X.X.</li>
</ol>
<b>Note: DURING FLASHING THE BOARD, THE UART CONNECTION MUST BE DISCONNECTED.</b>
<br><br>

<b>Setting Up Wi-Fi:</b><br>
Add or remove if you need
```C++
WifiNetwork wifiNetworks[] = {
  {"SSID1", "PASSWORD1", IPAddress(192, 168, 1, 100)},
  {"SSID2", "PASSWORD2", IPAddress(192, 168, 1, 101)},
  {"SSID3", "PASSWORD3", IPAddress(192, 168, 1, 102)}
};
```

<br>
<b>Go by ip in Serial Monitor::</b><br>

```C++
Connecting to WiFi: Name
.........
Connected to WiFi
192.168.0.56
```
