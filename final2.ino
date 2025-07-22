#include <WiFiNINA.h>
#include <DHT.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// Credenciales Wifi
char ssid[] = "FCDS_DORMS";
char pass[] = "fcds157usr";

//configuración thinkspeak
String apiKey = "I8FBHJS2XKA7KOOJ";
const char* server = "api.thingspeak.com";

// DHT11 
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// TinyGPS++ 
static const int RXPin = 4, TXPin = 3;
static const uint32_t GPSBaud = 9600;

TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

WiFiClient client;

// sensor vibración
const int vib_pin = 7;

// ultrasonico
const int trigPin = 10;
const int echoPin = 11;
const int hw505Pin = 8;

void setup() {
    Serial.begin(9600);
    delay(1000);

    dht.begin();
    ss.begin(GPSBaud);
    pinMode(vib_pin, INPUT);
    pinMode(hw505Pin, INPUT);

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    Serial.print("Connecting to WiFi");
    int status = WL_IDLE_STATUS;
    while (status != WL_CONNECTED) {
        status = WiFi.begin(ssid, pass);
        delay(5000);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void loop() {
    // leer dht 11
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        delay(2000);
        return;
    }

    // leer gps
    double latitude = 0.0;
    double longitude = 0.0;

    unsigned long start = millis();
    while (millis() - start < 1000) {
        while (ss.available() > 0) {
            gps.encode(ss.read());
            if (gps.location.isUpdated()) {
                latitude = gps.location.lat();
                longitude = gps.location.lng();
            }
        }
    }

    // leer sensor de vibración
    int vibrationState = digitalRead(vib_pin);
    int vibrationValue = (vibrationState == LOW) ? 1 : 0;

    // Leer sensor ultrasonico
    long duration;
    float distance;
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout
    distance = duration * 0.034 / 2.0; // cm

    // Sensor de inclinacion HW-505
    int hw505Value = digitalRead(hw505Pin);

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.print(" %, Latitude: ");
    Serial.print(latitude, 6);
    Serial.print(", Longitude: ");
    Serial.print(longitude, 6);
    Serial.print(", Vibración: ");
    Serial.print(vibrationValue);
    Serial.print(", Distancia: ");
    Serial.print(distance);
    Serial.print(" cm, HW-505: ");
    Serial.println(hw505Value);

    if (client.connect(server, 80)) {
        String url = "/update?api_key=" + apiKey +
                     "&field1=" + String(temperature) +
                     "&field2=" + String(humidity) +
                     "&field3=" + String(latitude, 6) +
                     "&field4=" + String(longitude, 6) +
                     "&field5=" + String(vibrationValue) +
                     "&field6=" + String(distance) +
                     "&field7=" + String(hw505Value);

        Serial.print("Requesting URL: ");
        Serial.println(url);

        client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                     "Host: " + server + "\r\n" +
                     "Connection: close\r\n\r\n");

        unsigned long timeout = millis();
        while (client.available() == 0) {
            if (millis() - timeout > 5000) {
                Serial.println(">>> Client Timeout !");
                client.stop();
                return;
            }
        }

        while (client.available()) {
            String line = client.readStringUntil('\r');
            Serial.print(line);
        }

        Serial.println("\nData sent to ThingSpeak.");
    } else {
        Serial.println("Failed to connect to ThingSpeak.");
    }

    client.stop();
    delay(20000);
}
