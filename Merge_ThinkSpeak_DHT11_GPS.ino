#include <WiFiNINA.h>
#include <DHT.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// WiFi credentials
char ssid[] = "FCDS_DORMS";
char pass[] = "fcds157usr";

// ThingSpeak settings
String apiKey = "I8FBHJS2XKA7KOOJ";
const char* server = "api.thingspeak.com";

// DHT11 settings
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// TinyGPS++ settings
static const int RXPin = 4, TXPin = 3;
static const uint32_t GPSBaud = 9600;

TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

WiFiClient client;

void setup() {
    Serial.begin(9600);
    delay(1000);

    dht.begin();
    ss.begin(GPSBaud);

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
    // Read DHT
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        delay(2000);
        return;
    }

    // Read GPS
    double latitude = 0.0;
    double longitude = 0.0;

    unsigned long start = millis();
    while (millis() - start < 1000) {  // collect GPS data for up to 1 second
        while (ss.available() > 0) {
            gps.encode(ss.read());
            if (gps.location.isUpdated()) {
                latitude = gps.location.lat();
                longitude = gps.location.lng();
            }
        }
    }

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.print(" %, Latitude: ");
    Serial.print(latitude, 6);
    Serial.print(", Longitude: ");
    Serial.println(longitude, 6);

    if (client.connect(server, 80)) {
        String url = "/update?api_key=" + apiKey +
                     "&field1=" + String(temperature) +
                     "&field2=" + String(humidity) +
                     "&field3=" + String(latitude, 6) +
                     "&field4=" + String(longitude, 6);

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
