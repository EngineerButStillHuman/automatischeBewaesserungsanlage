#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <WiFiUdp.h>
#include <ArduinoOTA.h>
 
const char* SSID = "UPC4866829";
const char* PSK = "zGkef2ecr8bb";
const char* MQTT_BROKER = "192.168.0.87";

#define SensorPin A0 
 
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
 
int relaiPin = 5; // D1 power pin 
float moisture_raw, moisture_percentage; // moisture measurement value

void setup_wifi();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();

float airValue = 736;
float waterValue = 290;

 
void setup() {   
    pinMode(relaiPin, OUTPUT);

    Serial.begin(115200);
    setup_wifi();
    client.setServer(MQTT_BROKER, 1883);
    client.setCallback(callback);

    // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
 
void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(SSID);
 
    WiFi.begin(SSID, PSK);
 
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
 
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

float read_moisture() {
  float moisture = analogRead(SensorPin);
  Serial.println(moisture);
  return moisture;
}

int calculate_percentage(float measurement) {
  float moisture = 100 - (100/(airValue-waterValue)*(measurement-waterValue));
  Serial.println(moisture);
  return moisture;
}
 
void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Received message ["); Serial.print(topic); Serial.print("] ");
    char msg[length+1];
    for (int i = 0; i < length; i++) {
        Serial.println((char)payload[i]);
        msg[i] = (char)payload[i];
    }
 
    msg[length] = '\0';
    //Serial.println(msg);


    if(strcmp(topic, "/Tomate/pumping")==0){
    if(strcmp(msg,"1")==0){
        Serial.println("Pumping");
        //Relais einschalten
          digitalWrite(relaiPin, HIGH);
        //2 Sekunden anlassen
          delay(10000);
        //Relais ausschalten  
          digitalWrite(relaiPin, LOW);
    }
    else{
        Serial.println("Not pumping");
    }
    }
    delay(1000);
    moisture_raw = read_moisture();
    moisture_percentage = calculate_percentage(moisture_raw);
    Serial.print("Current moisture: ");
    Serial.println(moisture_percentage);
    if (client.publish("/Tomate/moisture", String(moisture_percentage).c_str())) {
    Serial.println("Moisture sent!");
    } 
}
 
void reconnect() {
    while (!client.connected()) {
        Serial.println("Reconnecting MQTT...");
        if (!client.connect("Tomate")) {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" retrying in 5 seconds");
            delay(5000);
        }
    }
    client.subscribe("/Tomate/pumping");
    Serial.println("MQTT Connected...");
}

void loop() {
  ArduinoOTA.handle();
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
}