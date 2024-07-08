/*
 
  Sense distance and report distance in CM and inches when queried remotely via a web GET request.

  Using the ESP32-WROOM-DA Module
  Obtained from here: https://github.com/espressif/arduino-esp32/tree/master

  Acknowledge various sources for Sensor pin out info.
  Thanks to Seeed Grove for selling the hardware....



  Sysnopsis:

  Records distance to a surface every 20 seconds and stores it in a FIFO list of a fixed MAX length (due to memory constraints) for later retrieval.
  Currently there is no persistence across boot
  An HTTP get will retrieve and clear teh FIFObugffer
  Will wake, connect to the network via WIFI and rely on the DHCP router fixing its address based on this devices MAC address which is displayed on setup.


  TODO: Add an LCD display for ease of use.

 */

#include <WiFi.h>
#include <MySSIDSecrets.h>  // supplies these two variables with the real credential values and is stored in the arduino libraries folders area.
//const char *ssid = "SECRET_SSID";
//const char *password = "SECRETS_SSID_PASSWORD";
// (follow instructions here: https://forum.arduino.cc/t/how-to-store-common-secret-values-across-programs/1138023/2)
const unsigned long sensorTimeout = 10000000L;  // timeout afterwhich sensor data is regarded as bad
unsigned long sensorDelay = 0L;                 // variable from sensor

const int sensorTriggerPin = 5;
const int sensorEchoPin = 18;

float sensorDistanceCM;



//define sound speed in cm/uS
#define SOUND_SPEED 0.034

// define the server on port 80
NetworkServer server(80);

void setup() {

  Serial.begin(115200);

  initSensor();

  connect();

  server.begin();

}

void loop() {
  delay(1000);

  sensorDelay = pollSensor();
  storeValues(sensorDelay);

  NetworkClient client = server.accept();  // listen for incoming clients

  if (client) {                     // if you get a client,
    Serial.println("New Client.");  // print a message out the serial port
    String currentLine = "";        // make a String to hold incoming data from the client
                                    // while (sensorRetries < sensorMaxTries) {
    while (client.connected()) {    // loop while the client's connected
      if (client.available()) {     // if there's bytes to read from the client,
        char c = client.read();     // read a byte, then
        Serial.write(c);            // print it out the serial monitor
        if (c == '\n') {            // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            Serial.println("Sending response...");
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Distance (cm) ");
            client.println(sensorDistanceCM);


            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {  // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // close the connection:

    client.stop();
    Serial.println("Client Disconnected.");
  }
}

void initSensor() {
  pinMode(sensorTriggerPin, OUTPUT);  // Sets the trigPin as an Output
  pinMode(sensorEchoPin, INPUT);      // Sets the echoPin as an Input
  delay(10);
}
void connect() {
  // We start by connecting to a WiFi network

  requestConnection();

  waitForConnectionLoop();

  announceConnection();
}

void requestConnection() {
  Serial.println();
  Serial.print("Requesting cx to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
}

void waitForConnectionLoop() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
}
void announceConnection() {
  Serial.println("WiFi connected.");
  Serial.print("A: ");
  Serial.println(WiFi.localIP());
}

unsigned long pollSensor() {
  Serial.println("Polling sensor");
  // Clears the trigPin
  digitalWrite(sensorTriggerPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(sensorTriggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(sensorTriggerPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  unsigned long sensorValue = pulseIn(sensorEchoPin, HIGH, sensorTimeout);

  Serial.print("Sensor Delay ");
  Serial.println(sensorValue);
  return sensorValue;
}

void storeValues(long delay) {
  // Calculate the distance
  sensorDistanceCM = delay * SOUND_SPEED / 2;  
  Serial.print(" cm: ");
  Serial.println(sensorDistanceCM);
  
}
