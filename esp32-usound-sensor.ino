#include <EEPROM.h>

#include <Ethernet.h>

/*
 
 
  
 */
// #include <WiFi.h>
#include <Ethernet.h>
#include <NTPClient.h>
// #include <WiFiUdp.h>
#include <WebServer.h>
// #include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <EEPROM.h>
// #include "MyWIFI.h"

// Ultrasonic Sensor Pins
#define TRIG_PIN 5
#define ECHO_PIN 18

// LCD display setup
// LiquidCrystal_I2C lcd(0x27, 16, 2);

// Buzzer and LED Pins
#define BUZZER_PIN 4
#define LED_PIN 2

// SD Card Chip Select
const int chipSelect = 5;

// NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // Update interval set to 60 seconds

// Define the sensor data structure
struct SensorData {
  float value;
  unsigned long timestamp;
};

// Pointer to the dynamically allocated array
SensorData* sensorDataArray = nullptr;
int dataSize = 0; // Size of the array
int dataIndex = 0;

// Web server on port 80
WebServer server(80);

// Mutex for synchronizing access to the sensor data array
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// Function to read ultrasonic sensor data
float readUltrasonicSensor() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  if (duration == 0) {
    Serial.println("Failed to read from ultrasonic sensor");
    return -1; // Indicate a failure
  }
  float distance = duration * 0.034 / 2; // Convert to cm
  return distance;
}

// Function to get available free memory
int getFreeMemory() {
  // Simulate free memory function for ESP32
  return ESP.getFreeHeap();
}

// Function to format the timestamp into human-readable format
String formatTimestamp(unsigned long epochTime) {
  time_t rawTime = (time_t)epochTime;
  struct tm *timeInfo = localtime(&rawTime);
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeInfo);
  return String(buffer);
}

// Function to calculate average value
float calculateAverage() {
  float sum = 0;
  for (int i = 0; i < dataIndex; i++) {
    sum += sensorDataArray[i].value;
  }
  return sum / dataIndex;
}

// Function to calculate moving average
float movingAverage(float newValue) {
  static float values[10];
  static int index = 0;
  static int count = 0;
  values[index] = newValue;
  index = (index + 1) % 10;
  count = min(count + 1, 10);
  float sum = 0;
  for (int i = 0; i < count; i++) {
    sum += values[i];
  }
  return sum / count;
}

// // Function to log data to SD card
// void logData(float value, unsigned long timestamp) {
//   File dataFile = SD.open("datalog.txt", FILE_WRITE);
//   if (dataFile) {
//     dataFile.print("Value: ");
//     dataFile.print(value);
//     dataFile.print(", Timestamp: ");
//     dataFile.println(timestamp);
//     dataFile.close();
//   } else {
//     Serial.println("Error opening datalog.txt");
//   }
// }

// Function to connect to the best WiFi SSID
void connectToBestSSID() {
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("No networks found");
    return;
  }

  int bestRSSI = -1000;
  String bestSSID = "";
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    int rssi = WiFi.RSSI(i);
    if (rssi > bestRSSI) {
      bestRSSI = rssi;
      bestSSID = ssid;
    }
  }

  WiFi.begin(bestSSID.c_str(), password);
  Serial.print("Connecting to ");
  Serial.println(bestSSID);
  const int maxRetries = 10;
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < maxRetries) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi");
    // lcd.setCursor(0, 1);
    // lcd.print("WiFi Failed");
    // digitalWrite(BUZZER_PIN, HIGH); // Turn on the buzzer
    delay(500); // Buzzer sound for 500ms
    // digitalWrite(BUZZER_PIN, LOW); // Turn off the buzzer
  } else {
    Serial.println("Connected to Wi-Fi");
    Serial.println(WiFi.localIP());
    // lcd.setCursor(0, 1);
    // lcd.print(WiFi.localIP());
    // digitalWrite(LED_PIN, HIGH); // Turn on the LED if WiFi is connected
  }
}

void handleDataRequest() {
  String response = "[";
  portENTER_CRITICAL(&mux); // Enter critical section
  for (int i = 0; i < dataIndex; i++) {
    response += "{\"value\":" + String(sensorDataArray[i].value) + ",\"timestamp\":" + String(sensorDataArray[i].timestamp) + "}";
    if (i < dataIndex - 1) {
      response += ",";
    }
  }
  response += "]";
  dataIndex = 0; // Clear the array
  portEXIT_CRITICAL(&mux); // Exit critical section
  server.send(200, "application/json", response);
}

void handleRootRequest() {
  const char* htmlPage = R"rawliteral(
  <!DOCTYPE HTML><html>
  <head><title>Sensor Data</title></head>
  <body>
  <h1>Sensor Data</h1>
  <table border="1">
    <tr><th>Timestamp</th><th>Distance (cm)</th></tr>
    <tbody id="dataTable"></tbody>
  </table>
  <script>
  async function fetchData() {
    const response = await fetch('/data');
    const data = await response.json();
    const table = document.getElementById('dataTable');
    table.innerHTML = '';
    data.forEach(entry => {
      const row = document.createElement('tr');
      const timeCell = document.createElement('td');
      const valueCell = document.createElement('td');
      timeCell.textContent = new Date(entry.timestamp * 1000).toLocaleString();
      valueCell.textContent = entry.value.toFixed(2);
      row.appendChild(timeCell);
      row.appendChild(valueCell);
      table.appendChild(row);
    });
  }
  setInterval(fetchData, 1000);
  </script>
  </body></html>)rawliteral";
  server.send(200, "text/html", htmlPage);
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // // Initialize the LCD
  // lcd.begin();
  // lcd.backlight();

  // Initialize Ultrasonic Sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Initialize Buzzer and LED
  // pinMode(BUZZER_PIN, OUTPUT);
  // pinMode(LED_PIN, OUTPUT);

  // Connect to the best WiFi SSID
  connectToBestSSID();

  Serial.println("Initialize the NTP client");
  timeClient.begin();

  Serial.println("// Get free memory and adjust array size");
  
  int freeMem = getFreeMemory();
  dataSize = freeMem / sizeof(SensorData) / 5; 
  Serial.print("Use 1 fifth of the free memory: ");
  Serial.println(dataSize);
  sensorDataArray = new SensorData[dataSize];
  
  Serial.print("Array size: ");
  Serial.println(dataSize);

  // Initialize SD card
  // if (!SD.begin(chipSelect)) {
  //   Serial.println("SD card initialization failed!");
  //   return;
  // }
  // Serial.println("SD card is ready to use.");

  // Serial.println("Setup server to handle GET request");
  server.on("/data", HTTP_GET, handleDataRequest);

  // Serial.println("Serve HTML page");
  server.on("/", HTTP_GET, handleRootRequest);

  Serial.println("Start server");
  server.begin();
}

void loop() {
  // Serial.println("Handle client requests");
  server.handleClient();

  // Serial.println("Update the NTP client to get the current time");
  timeClient.update();

  // Serial.println("Read the sensor value every second");
  static unsigned long lastMillis = 0;
  if (millis() - lastMillis >= 10000) {
    lastMillis = millis();

    // Read the sensor value
    float sensorValue = readUltrasonicSensor();

    // Get the current timestamp
    unsigned long currentTime = timeClient.getEpochTime();

    // Store the sensor value and timestamp in the array
    portENTER_CRITICAL(&mux); // Enter critical section
    if (dataIndex < dataSize) {
      sensorDataArray[dataIndex].value = sensorValue;
      sensorDataArray[dataIndex].timestamp = currentTime;
      dataIndex++;
    } else {
      // Shift data for FIFO
      for (int i = 0; i < dataSize - 1; i++) {
        sensorDataArray[i] = sensorDataArray[i + 1];
      }
      sensorDataArray[dataSize - 1].value = sensorValue;
      sensorDataArray[dataSize - 1].timestamp = currentTime;
    }
    portEXIT_CRITICAL(&mux); // Exit critical section

    // Log data to SD card
    // logData(sensorValue, currentTime);
  }
}
