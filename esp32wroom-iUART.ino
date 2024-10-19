#include <WiFi.h>
#include <WebServer.h>
#include <HardwareSerial.h>

// Wi-Fi Credentials and Static IPs
struct WifiNetwork {
  const char* ssid;
  const char* password;
  IPAddress staticIP;
};

WifiNetwork wifiNetworks[] = {
  {"SSID1", "PASSWORD1", IPAddress(192, 168, 1, 100)},
  {"SSID2", "PASSWORD2", IPAddress(192, 168, 1, 101)},
  {"SSID3", "PASSWORD3", IPAddress(192, 168, 1, 102)}
};

WebServer server(80);

// Setting up UARTs
HardwareSerial MySerial1(1);  // Use UART1
HardwareSerial MySerial2(2);  // Use UART2
HardwareSerial* activeSerial = &MySerial1;  // Pointer to the active UART
int RX1_PIN = 16;  // RX pin for UART1
int TX1_PIN = 17;  // TX pin for UART1
int RX2_PIN = 25;  // RX pin for UART2
int TX2_PIN = 26;  // TX pin for UART2

String logs = "";  // Line for storing logs
bool newData = false;  // Flag for checking new data
int udhcpcCount = 0;  // Counter for udhcpc discover messages

void setup() {
  Serial.begin(115200);  // Arduino IDE Debug Console
  MySerial1.begin(115200, SERIAL_8N1, RX1_PIN, TX1_PIN);  // UART1 for LuckFox
  MySerial2.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);  // UART2 for other device

  // Attempt to connect to Wi-Fi networks in order
  for (int i = 0; i < sizeof(wifiNetworks) / sizeof(wifiNetworks[0]); i++) {
    WiFi.begin(wifiNetworks[i].ssid, wifiNetworks[i].password);
    WiFi.config(wifiNetworks[i].staticIP, IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    Serial.print("Connecting to WiFi: ");
    Serial.println(wifiNetworks[i].ssid);

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to WiFi");
      Serial.println(WiFi.localIP());
      break;
    } else {
      Serial.println("\nFailed to connect");
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Could not connect to any WiFi network");
  }

  // Setting up the web interface
  server.on("/", handleRoot);  // Home page
  server.on("/logs", handleLogs);  // Logs on request
  server.on("/send", handleSend);  // Command to send data via UART
  server.on("/select_uart", handleSelectUART);  // Select UART
  server.on("/style.css", handleStyle);
  server.on("/script.js", handleScript);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Working with a web server
  server.handleClient();
  
  // Reading data from active UART
  if (activeSerial->available()) {
    String tempLogs = "";
    while (activeSerial->available()) {
      char data = activeSerial->read();  // Reading data one character at a time
      tempLogs += data;  // Adding a symbol to the temporary logs
    }
    
    // Check if the logs contain only udhcpc discover message
    if (tempLogs.indexOf("udhcpc: broadcasting discover") != -1 && tempLogs.length() == 31) {
      udhcpcCount++;
    } else {
      logs += tempLogs;  // Adding the logs if it's not just the udhcpc message
      newData = true;  // Set the new data flag
      Serial.println("Received from UART: " + tempLogs);  // Output data to console
    }
  }
}

// Processing a web request (main page)
void handleRoot() {
  String html = R"rawliteral(
  <html>
  <head>
    <link rel='stylesheet' type='text/css' href='/style.css'>
    <script src='/script.js'></script>
  </head>
  <body class='main-body'>
    <header id='header' class='main-header'>
      <b class='header-title'>iUART Logs</b>
      <a href='https://en.wikipedia.org/wiki/Universal_asynchronous_receiver-transmitter' target='_blank' class='header-link'>What is UART?</a>
      <p class='header-info'>UDHCPC Discover Count: <span id='udhcpc-count'>0</span></p>
      <p class='header-info active-uart'>Active UART: <span id='active-uart'>UART1</span></p>
    </header>
    <div id='logs' class='logs'></div>
    <div id='input-form' class='input-form'>
      <div class='button-container'>
        <button id='send-button' class='command-button'>Send</button>
        <button id='enter-button' class='command-button'>Enter</button>
        <button id='backspace-button' class='command-button'>Backspace</button>
        <button id='ctrlc-button' class='command-button'>Ctrl+C</button>
        <button id='tab-button' class='command-button'>Tab</button>
        <button id='switch-uart-button' class='command-button'>Switch UART</button>
      </div>
      <textarea id='command-input' class='command-input' placeholder='Enter command here'></textarea>
    </div>
  </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

// Sending logs to a web page
void handleLogs() {
  String response = logs;
  response += "\nUDHCPC Discover Count: " + String(udhcpcCount);
  if (newData) {  // We send only if there is new data
    server.send(200, "text/plain", response);  // Sending accumulated logs
    newData = false;  // Drop the flag
  } else {
    server.send(200, "text/plain", "");  // No new data
  }
}

// Processing a command from the web interface and sending it via UART
void handleSend() {
  if (server.hasArg("command")) {
    String command = server.arg("command");
    Serial.println("Sending command to UART: " + command);
    
    activeSerial->print(command);  // Sending a command to active UART without extra newline
    server.send(200, "text/plain", "Command sent: " + command);
  } else {
    server.send(200, "text/plain", "No command received");
  }
}

// Handle UART selection
void handleSelectUART() {
  if (server.hasArg("uart")) {
    String uart = server.arg("uart");
    if (uart == "UART1") {
      activeSerial = &MySerial1;
      server.send(200, "text/plain", "Switched to UART1");
    } else if (uart == "UART2") {
      activeSerial = &MySerial2;
      server.send(200, "text/plain", "Switched to UART2");
    } else {
      server.send(400, "text/plain", "Invalid UART selection");
    }
  } else {
    server.send(400, "text/plain", "No UART specified");
  }
}

// Additional endpoints for JavaScript and CSS
void handleStyle() {
  String css = R"rawliteral(
  .main-body {
    margin: 0;
    padding: 0;
    background-color: #ffffff;
    color: black;
    font-family: monospace;
  }
  .main-header {
    display: flex;
    justify-content: center;
    align-items: end;
    padding-top: 20px;
    width: 100%;
    text-align: center;
    background: linear-gradient(to top, rgba(172, 172, 200, 1), rgba(255, 255, 255, 1) 64.71%);
  }
  .header-title, .header-link, .header-info {
    margin: 0;
    border: 1px solid black;
    border-bottom: none;
    border-radius: 5px 5px 0px 0px;
    background: linear-gradient(0deg, rgba(255,255,255,1) 0%, rgba(203,203,203,1) 35%, rgba(255,255,255,1) 100%);
    padding: 5px;
  }
  .header-title {
    color: rgb(65, 65, 255);
  }
  .active-uart {
    color: rgb(65, 65, 255);
  }
  .logs {
    white-space: pre-wrap;
    height: calc(100vh - 360px);
    overflow-y: scroll;
    padding: 20px;
    background: linear-gradient(rgba(203, 203, 203, 1), rgba(255, 255, 255, 1) 2.1%, rgb(255, 255, 255) 97.48%, rgba(203, 203, 203, 1));
  }
  .input-form {
    display: flex;
    flex-direction: column;
    height: 250px;
    background: linear-gradient(to top, rgba(255, 255, 255, 1), rgb(255, 255, 255) 80.25%, rgba(173, 173, 173, 1) 89.92%, rgb(255, 255, 255));
    padding: 10px;
  }
  .button-container {
    margin-top: 10px;
  }
  .command-button {
    padding: 10px;
    color: black;
    cursor: pointer;
    margin-right: 10px;
    background: linear-gradient(to top, rgba(255, 255, 255, 1) 20%, rgba(213, 213, 213, 1) 50%, rgb(255, 255, 255) 80%);
    border: 2px solid black;
    border-radius: 7px;
    font-weight: 600;
    box-shadow: 0 0 9px rgba(0, 0, 0, 0.31);
  }
  .command-button:hover {
    background-color: #888;
  }
  .command-input {
    flex: 1;
    padding: 10px;
    color: black;
    border: 2px solid black;
    height: 60px;
    resize: none;
    margin: 10px 5px;
    border-radius: 10px;
    margin-left: 0;
    box-shadow: 0 0 9px rgba(0, 0, 0, 0.31);
  }
  )rawliteral";
  server.send(200, "text/css", css);
}

void handleScript() {
  String js = R"rawliteral(
  document.addEventListener('DOMContentLoaded', function() {
    let logsCache = '';
    function fetchLogs() {
      fetch('/logs').then(r => r.text()).then(t => {
        if (t !== logsCache) {
          const logsData = t.split('\n');
          const udhcpcCount = logsData.pop().split(': ')[1];
          document.getElementById('logs').innerHTML = logsCache + logsData.join('\n');
          document.getElementById('udhcpc-count').innerText = udhcpcCount || 0;
          logsCache = logsData.join('\n');
          document.getElementById('logs').scrollTop = document.getElementById('logs').scrollHeight;  // Scroll to the bottom
        }
      });
    }
    function sendCommand() {
      const command = document.getElementById('command-input').value;
      fetch('/send?command=' + encodeURIComponent(command))
        .then(() => {
          document.getElementById('command-input').value = '';
          fetchLogs();
        });
    }
    document.getElementById('command-input').addEventListener('keydown', function(event) {
      if (event.key === 'Enter' && !event.shiftKey) {
        event.preventDefault();
        sendCommand();
      }
    });
    document.getElementById('send-button').addEventListener('click', function() {
      sendCommand();
    });
    document.getElementById('enter-button').addEventListener('click', function() {
      fetch('/send?command=' + encodeURIComponent('\n'))
        .then(() => {
          fetchLogs();
        });
    });
    document.getElementById('backspace-button').addEventListener('click', function() {
      fetch('/send?command=' + encodeURIComponent('\b'))
        .then(() => {
          fetchLogs();
        });
    });
    document.getElementById('ctrlc-button').addEventListener('click', function() {
      fetch('/send?command=' + encodeURIComponent(String.fromCharCode(3)))
        .then(() => {
          fetchLogs();
        });
    });
    document.getElementById('tab-button').addEventListener('click', function() {
      fetch('/send?command=' + encodeURIComponent('\t'))
        .then(() => {
          fetchLogs();
        });
    });
    document.getElementById('switch-uart-button').addEventListener('click', function() {
      const activeUART = document.getElementById('active-uart').innerText;
      const newUART = activeUART === 'UART1' ? 'UART2' : 'UART1';
      fetch('/select_uart?uart=' + newUART)
        .then(() => {
          document.getElementById('active-uart').innerText = newUART;
          fetchLogs();
        });
    });
    setInterval(fetchLogs, 2000);
  });
  )rawliteral";
  server.send(200, "application/javascript", js);
}
