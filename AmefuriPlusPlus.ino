// set up for Temp and Humdity 
#include "DHT.h"
#define DHTPIN 2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

//set up for Wind Counter
const int windSensor = A0;
unsigned long startTime;
const unsigned long minute = 60000; // 60000 ms
int windCount = 0;

void setup() {

  Serial.begin(9600);
  pinMode(windSensor, INPUT);
  pinMode(DHTPIN, OUTPUT);
  startTime = millis();

  dht.begin();
}

void tempAndHumid() {
  // Wait a few seconds between measurements.
  delay(1000);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  // if (isnan(h) || isnan(t) || isnan(f)) {
  //   Serial.println(F("Failed to read from DHT sensor!"));
  //   return;
  // }

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
}

void windCounter() {
  int isCount = digitalRead(windSensor);
  if (millis() - startTime < minute) {
      if (isCount) {
      windCount++;
      Serial.println(windCount);
      delay(1000);
    }
  } else {
    if (windCount < 4) {
      Serial.println("0 Round(s) Per Minute(s)");
    } else {
      Serial.print(int(windCount / 3.5) + 1);
      Serial.println(" Round(s) Per Minute(s)");
    }
    
    startTime = millis();
    windCount = 0;
  }
}

void loop() {
  // windCounter();
  tempAndHumid();
}