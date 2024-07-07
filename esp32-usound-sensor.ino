/*
 
  Sense distance and report distance in CM and inches when queried remotely via a web GET request.

  Using the ESP32-WROOM-DA Module
  Obtained from here: https://github.com/espressif/arduino-esp32/tree/master

  Acknowledge various sources for Sensor pin out info.
  Thanks to Seeed Grove for selling the hardware....

 */

#include <WiFi.h>
#include <MySSIDSecrets.h>  // supplies these two variables with the real credential values
// (follow instructions here: https://forum.arduino.cc/t/how-to-store-common-secret-values-across-programs/1138023/2)
//const char *ssid = "SECRET_SSID";
//const char *password = "SECRETS_SSID_PASSWORD";

const unsigned long sensorTimeout = 10000000L;  // timeout afterwhich sensor data is regarded as bad
unsigned long sensorDelay = 0L;                 // variable from sensor
const unsigned int sensorMaxTries = 10;

const int sensorTriggerPin = 5;
const int sensorEchoPin = 18;
float sensorDistanceCm;
float sensorDistanceInch;

//define sound speed in cm/uS
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

NetworkServer server(80);

void setup() {
  Serial.begin(115200);

  initSensor();

  delay(10);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {

  unsigned int sensorRetries = 0;

  NetworkClient client = server.accept();  // listen for incoming clients

  if (client) {                     // if you get a client,
    Serial.println("New Client.");  // print a message out the serial port
    String currentLine = "";        // make a String to hold incoming data from the client
                                    // while (sensorRetries < sensorMaxTries) {
    sensorDelay = pollSensor();
    Serial.print("Sensor Delay ");
    Serial.println(sensorDelay);
    //   if (sensorDelay < sensorTimeout) break;
    //   sensorRetries++;
    // }
    if (sensorRetries < sensorMaxTries) {
      calculateDistance(sensorDelay);
      while (client.connected()) {  // loop while the client's connected
        if (client.available()) {   // if there's bytes to read from the client,
          char c = client.read();   // read a byte, then
          Serial.write(c);          // print it out the serial monitor
          if (c == '\n') {          // if the byte is a newline character

            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();

              // the content of the HTTP response follows the header:
              client.print("Distance (cm) ");
              client.println(sensorDistanceCm);
              client.print("Distance (in) ");
              client.println(sensorDistanceInch);


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
    }
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

void initSensor() {
  pinMode(sensorTriggerPin, OUTPUT);  // Sets the trigPin as an Output
  pinMode(sensorEchoPin, INPUT);      // Sets the echoPin as an Input
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
  return pulseIn(sensorEchoPin, HIGH, sensorTimeout);
}

void calculateDistance(long delay) {
  // Calculate the distance
  sensorDistanceCm = delay * SOUND_SPEED / 2;

  // Convert to inches
  sensorDistanceInch = sensorDistanceCm * CM_TO_INCH;
  Serial.print("cm: ");
  Serial.println(sensorDistanceCm);
  Serial.print("in: ");
  Serial.println(sensorDistanceInch);
}
