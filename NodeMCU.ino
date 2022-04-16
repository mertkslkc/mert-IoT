//////////////////////////////////////////////////////////////////
///                                                            ///
///               This code belongs to MERT KIŞLAKÇI           ///
/// Contact: mertkslkc@gmail.com | linkedin.com/in/mert-kslkc  ///
///                                                            ///
//////////////////////////////////////////////////////////////////

/////////////////////////  Library  ////////////////////////
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "ACS712.h"

//////////////////////// Global Args //////////////////////
unsigned long lastMsg = 0;
const char* ssid = "Enter your wifi name here";
const char* password = "Enter your wifi password here";
const char* mqtt_server = "Enter your mqtt address here";

////////////////  Current Sensor Config  /////////////////
// ACS712 5A  uses 185 mV per A
// ACS712 20A uses 100 mV per A
// ACS712 30A uses  66 mV per A
ACS712  ACS(A0, 5.0, 1023, 66); //ACS(Nodemcu ADC PIN, ACS supply voltage, ADC resolution nodemcu this is 1023 kind, ACS calibration value);

////////////////  Client Config  /////////////////
WiFiClient espClient; // Creates a client that can connect to to a specified internet IP address and port as defined in client.connect()
PubSubClient client(espClient); // Creates a fully configured client instance


void setup_wifi() { // In case of a possible disconnection, this function will try to connect again.

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);

  WiFi.mode(WIFI_STA); //Station mode: the ESP connects to an access point
  WiFi.begin(ssid, password); // Initializes the WiFi library’s network settings and provides the current status.

  while (WiFi.status() != WL_CONNECTED) { // When the esp connects to wifi , the WiFi.status() function returns WL_CONNECTED and if it is not equal to this expression, it will try to connect again.
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); // your network address
}

// If the client is used to subscribe to topics, a callback function must be provided in the constructor. This function is called when new messages arrive at the client.
// topic const char[] - the topic the message arrived on
// payload byte[] - the message payload
// length unsigned int - the length of the message payload
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // If the incoming message is 1, it turns on the lamp.
  if ((char)payload[0] == '1') {
    digitalWrite(D1, HIGH);
  }
  // If the incoming message is 1, it turns off the lamp.
  else {
    digitalWrite(D1, LOW);
  }
}

void reconnect() {

  // If the connection is lost, it will try to connect again.
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Subscribe to the incoming message with the lamp topic
      client.subscribe("lamp");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(D1, OUTPUT);  // We activated the nodemcu d1 output
  Serial.begin(115200); // communication band
  setup_wifi(); // setup_wifi function included
  client.setServer(mqtt_server, 1883); // We configured mqtt to communicate on port 1883.
  client.setCallback(callback); // It is used to call the message broadcast on the client side.
  ACS.autoMidPoint(); // AC average frequency
}

void loop() {

  if (!client.connected()) { // If the client is not connected to the mqtty, it tries to connect again with the reconnect function.
    reconnect();
  }
  client.loop(); // This should be called regularly to allow the client to process incoming messages and maintain its connection to the server.


  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    StaticJsonBuffer<800> JSON; // We have created a json object because in order to publish the stream values ​​properly
    JsonObject& JSONencoder = JSON.createObject();
    int mA = ACS.mA_AC(); // Reads current value in mA

    Serial.print("Publish message: ");
    Serial.println(mA);
    JSONencoder["Amper = "]  = mA;
    char JSONmessageBuffer[800];
    JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    client.publish("Room", JSONmessageBuffer); // send mqtt amp value to room topic

  }
}
