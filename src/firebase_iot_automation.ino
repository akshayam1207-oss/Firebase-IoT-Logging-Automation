```cpp
/*
  Project: Smart IoT Monitoring Dashboard
  Task 5: Firebase Logging, Automation and Data Export

  Hardware:
  - ESP32 DevKit V4
  - DHT11 temperature and humidity sensor
  - Analog LDR module
  - 5V relay module

  Pin connections:
  DHT11 data -> GPIO 4
  LDR analog output -> GPIO 34
  Relay IN -> GPIO 26

  Features:
  - Reads temperature and humidity
  - Reads ambient light using the LDR
  - Sends current readings to Firebase
  - Stores historical readings in Firebase
  - Supports manual bulb control
  - Supports automatic bulb control using an LDR threshold

  IMPORTANT:
  Replace the configuration placeholders with your own credentials.
  Never upload passwords or private database credentials to GitHub.
*/

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <DHT.h>

// ---------------- Wi-Fi Configuration ----------------

#define WIFI_SSID       "YOUR_WIFI_NAME"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

// ---------------- Firebase Configuration -------------

#define FIREBASE_HOST   "YOUR_FIREBASE_DATABASE_URL"
#define FIREBASE_AUTH   "YOUR_FIREBASE_DATABASE_SECRET"

// ---------------- Hardware Pins ----------------------

#define DHT_PIN         4
#define DHT_TYPE        DHT11
#define LDR_PIN         34
#define RELAY_PIN       26

// ---------------- Initial Settings -------------------

int ldrThreshold = 800;

bool bulbState = false;

String mode = "manual";

// ---------------- Firebase Objects -------------------

FirebaseData firebaseData;
FirebaseData streamData;

FirebaseJson json;

DHT dht(DHT_PIN, DHT_TYPE);

// ---------------- Timing -----------------------------

unsigned long previousMillis = 0;

const unsigned long readingInterval = 2000;

// ---------------- Function Declarations --------------

void connectWiFi();
void readSensorData();
void updateFirebase();
void readFirebaseSettings();
void controlBulb();
void firebaseStreamCallback(
  StreamData data
);
void firebaseStreamTimeoutCallback(
  bool timeout
);

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // Configure the hardware pins.
  pinMode(LDR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  // Start with the relay output LOW.
  digitalWrite(RELAY_PIN, LOW);

  // Initialize the DHT11 sensor.
  dht.begin();

  // Connect the ESP32 to Wi-Fi.
  connectWiFi();

  // Initialize Firebase.
  Firebase.begin(
    FIREBASE_HOST,
    FIREBASE_AUTH
  );

  Firebase.reconnectWiFi(true);

  // Start listening for dashboard control commands.
  if (Firebase.beginStream(
        streamData,
        "/control"
      )) {

    Firebase.setStreamCallback(
      streamData,
      firebaseStreamCallback,
      firebaseStreamTimeoutCallback
    );

    Serial.println(
      "Firebase control stream started."
    );

  } else {

    Serial.println(
      "Could not start Firebase stream."
    );

    Serial.println(
      streamData.errorReason()
    );
  }

  Serial.println(
    "Smart IoT Monitoring System Started."
  );
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  // Keep the Firebase stream active.
  if (Firebase.ready()) {

    if (!Firebase.readStream(streamData)) {

      Serial.println(
        streamData.errorReason()
      );
    }
  }

  // Read sensors and update Firebase periodically.
  unsigned long currentMillis = millis();

  if (
    currentMillis - previousMillis
    >= readingInterval
  ) {

    previousMillis = currentMillis;

    readSensorData();

    readFirebaseSettings();

    controlBulb();

    updateFirebase();
  }
}

// =====================================================
// CONNECT TO WI-FI
// =====================================================

void connectWiFi() {

  Serial.print(
    "Connecting to Wi-Fi"
  );

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  while (
    WiFi.status() != WL_CONNECTED
  ) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.println(
    "Wi-Fi connected."
  );

  Serial.print(
    "ESP32 IP Address: "
  );

  Serial.println(
    WiFi.localIP()
  );
}

// =====================================================
// READ SENSOR DATA
// =====================================================

float temperature = 0.0;

float humidity = 0.0;

int ldrValue = 0;

void readSensorData() {

  // Read temperature and humidity.
  float newTemperature =
    dht.readTemperature();

  float newHumidity =
    dht.readHumidity();

  // Update readings only when the DHT11
  // returns valid data.
  if (
    !is
