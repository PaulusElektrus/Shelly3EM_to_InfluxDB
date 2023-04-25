// Needed for PlatformIO
#include <Arduino.h>

// W-Lan & InfluxDB
#include <WiFi.h>
#include <InfluxDbClient.h>

#include <HTTPClient.h>
#include <ArduinoJson.h>

// Definitionen
#define DEVICE "ESP32"

// WLAN Zugangsdaten
#define WIFI_SSID "++++"
#define WIFI_PASSWORD "++++"

// InfluxDB
#define INFLUXDB_URL "++++"
#define INFLUXDB_DB_NAME "energy"
#define INFLUXDB_USER "++++"
#define INFLUXDB_PASSWORD "++++"

// InfluxDB
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);

// Variablen
Point sensor("energy");

 
//////////////////////////// SETUP

void setup() 
{
  // serieller Monitor
  Serial.begin(115200);
  Serial.println("Booting...");

  Serial.println("Verbinden mit ");
  Serial.println(WIFI_SSID);

  // W-Lan Verbindung
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // W-Lan prüfen 
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi verbunden..!");
  Serial.print("IP= ");  Serial.println(WiFi.localIP());

  // InfluxDB V 1.0
  client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);

  // Add Tags
  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());

  // Check InfluxDB connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  Serial.println("Start des Hauptprogramms");
}


//////////////////////////// HAUPTPROGRAMM

void loop() {
  sensor.clearFields();

  HTTPClient http; //Instanz von HTTPClient starten
    http.begin("http://192.168.0.121/rpc/EM.GetStatus?id=0"); //Abfrage-URL
    int httpCode = http.GET(); //Antwort des Servers abrufen
    if (httpCode == 200) {
      String payload = http.getString(); //Daten in eine Variable speichern
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      JsonObject root = doc.as<JsonObject>();

      for (JsonPair kv : root) {
        sensor.addField(kv.key().c_str(), kv.value().as<float>());
      }
    }

  // Print & Check
  Serial.print("Writing: ");
  Serial.println(client.pointToLineProtocol(sensor));
  // If no Wifi signal, try to reconnect it
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
  }
  
  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
  
  //Wait 1s
  Serial.println("Wait 1s");
  delay(1000);
}