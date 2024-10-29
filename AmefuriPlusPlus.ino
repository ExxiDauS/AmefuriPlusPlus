// setup for MQTT

#include <WiFiS3.h>
#include <MQTTClient.h>

const char WIFI_SSID[] = "Samsung Galaxy S24 Ultra";          // CHANGE TO YOUR WIFI SSID
const char WIFI_PASSWORD[] = "090123tn";  // CHANGE TO YOUR WIFI PASSWORD

const char MQTT_BROKER_ADRRESS[] = "mqtt-dashboard.com";  // CHANGE TO MQTT BROKER'S ADDRESS
const int MQTT_PORT = 1883;
const char MQTT_CLIENT_ID[] = "Kawin";  // CHANGE IT AS YOU DESIRE
const char MQTT_USERNAME[] = "";                                            // CHANGE IT IF REQUIRED, empty if not required
const char MQTT_PASSWORD[] = "";                                            // CHANGE IT IF REQUIRED, empty if not required

// The MQTT topics that Arduino should publish/subscribe
const char PUBLISH_TOPIC[] = "Amefuri/plus";    // CHANGE IT AS YOU DESIRE
const char SUBSCRIBE_TOPIC[] = "Amefuri/plus";  // CHANGE IT AS YOU DESIRE

const int PUBLISH_INTERVAL = 30000;  // 60 seconds

WiFiClient network;
MQTTClient mqtt = MQTTClient(256);

unsigned long lastPublishTime = 0;

// -------------------------------------------------------------------------------------------

// set up for Temp and Humdity 
#include "DHT.h"
#define DHTPIN 2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

//set up for Wind Counter
const int windSensor = A0;
unsigned long startTime;
int windCount = 0;

struct tempHumidInfo {
  float humid;
  float celsius;
  float farenheit;
  String message;
};

void setup() {

  Serial.begin(9600);
  pinMode(windSensor, INPUT);
  pinMode(DHTPIN, OUTPUT);

  dht.begin();

  // connect MQTT
  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    Serial.print("Arduino UNO R4 - Attempting to connect to SSID: ");
    Serial.println(WIFI_SSID);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // wait 10 seconds for connection:
    delay(10000);
  }
  // print your board's IP address:
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  connectToMQTT();
}

void loop() {
  static unsigned long startTime = 0;  // Start time for wind counting
  static bool counting = false;         // Flag for counting state
  const unsigned long duration = 60000; // 1 minute in milliseconds

  // Start counting wind events
  if (!counting) {
    startTime = millis(); // Record the start time
    counting = true;      // Set counting flag
    windCount = 0;        // Reset wind count
  }

  // Count wind events for one minute
  if (counting) {
    windCounter(); // Count wind events

    // Check if one minute has passed
    if (millis() - startTime >= duration) {
      counting = false; // Stop counting
      int windFullRound = (windCount < 4) ? 0 : (int(windCount / 3.5) + 1);
      Serial.print(windFullRound);
      Serial.println(" Round(s) Per Minute(s)");

      // Get temperature and humidity after counting wind
      struct tempHumidInfo info = tempAndHumid();
      String rain_status = checkStatus(windFullRound, info.humid);
      Serial.println(rain_status);

      // Send data to MQTT
      if (millis() - lastPublishTime > PUBLISH_INTERVAL) {
        sendToMQTT(rain_status);
        lastPublishTime = millis();
      }
    }
  }

  mqtt.loop(); // Keep the MQTT connection alive
}


// MQTT Functions


void connectToMQTT() {
  // Connect to the MQTT broker
  mqtt.begin(MQTT_BROKER_ADRRESS, MQTT_PORT, network);

  // Create a handler for incoming messages
  mqtt.onMessage(messageHandler);

  Serial.print("Arduino UNO R4 - Connecting to MQTT broker");

  while (!mqtt.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  if (!mqtt.connected()) {
    Serial.println("Arduino UNO R4 - MQTT broker Timeout!");
    return;
  }

  // Subscribe to a topic, the incoming messages are processed by messageHandler() function
  if (mqtt.subscribe(SUBSCRIBE_TOPIC))
    Serial.print("Arduino UNO R4 - Subscribed to the topic: ");
  else
    Serial.print("Arduino UNO R4 - Failed to subscribe to the topic: ");

  Serial.println(SUBSCRIBE_TOPIC);
  Serial.println("Arduino UNO R4 - MQTT broker Connected!");
}

void sendToMQTT(String val) {

  // Convert the string to a char array for MQTT publishing
  char messageBuffer[101];
  val.toCharArray(messageBuffer, 101);

  // Publish the message to the MQTT topic
  mqtt.publish(PUBLISH_TOPIC, messageBuffer);

  // Print debug information to the Serial Monitor in one line
  Serial.println("Arduino UNO R4 - sent to MQTT: topic: " + String(PUBLISH_TOPIC) + " | payload: " + String(messageBuffer));
}

void messageHandler(String &topic, String &payload) {
  Serial.println("Arduino UNO R4 - received from MQTT: topic: " + topic + " | payload: " + payload);

}


// Temp and Humid Function
tempHumidInfo tempAndHumid() {
  // Wait a few seconds between measurements.
  tempHumidInfo result;
  delay(1000);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));

    result.humid = 0;
    result.celsius = 0;
    result.farenheit = 0;
    result.message = "Failed to read from DHT sensor!";
    return result;
  }

  result.humid = h;
  result.celsius = t;
  result.farenheit = f;
  result.message = "Success";

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F(" C "));
  Serial.print(f);
  Serial.print(F(" F  Heat index: "));
  Serial.print(hic);
  Serial.print(F(" C "));
  Serial.print(hif);
  Serial.println(F(" F"));

  return result;
}

// Wind Counter Function
void windCounter() {
  int isCount = digitalRead(windSensor);
  if (isCount) {
    windCount++;
    Serial.println(windCount);
    delay(500); // Delay to avoid multiple counts from the same wind event
  }
}


// Check if rain or not
String checkStatus(int windFullRound, float humid) {
  if (windFullRound >= 3 && humid >= 80.00) {
    return "It's Raining, GO GET YOUR CLOTHS!!";
  } else {
    return "It's not Raining, Your cloths are safe";
  }
}
