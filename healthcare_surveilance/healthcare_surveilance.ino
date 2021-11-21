/*
 * HEALTHCARE SURVEILANCE SYSTEM
 * 18219040 - Rahmat Wibowo
 * 18219058 - Afif Fahreza
 * 18219076 - Yahya Aqrom
 */


// Include library yang dibutuhkan
#include <Wire.h>
#include <WiFi.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include <Adafruit_SSD1306.h>


// Credentials WIFI
const char* ssid     = "SSID";
const char* password = "PASSWORD";

// Variable sensor dan display
MAX30105 particleSensor;
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

// Domain name serta URL server database
const char* server = "backend-embedded-system.herokuapp.com";

uint32_t irBuffer[50]; //infrared LED sensor data
uint32_t redBuffer[50];  //red LED sensor data

int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator valid SPO2
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator valid heartrate



void setup() {
  Serial.begin(115200); 

  // Inisialisasi sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Default I2C port = 400kHz speed
  {
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    while (1);
  }

  particleSensor.setup(55, 4, 2, 200, 411, 4096); //Konfigurasi sensor

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("WiFi connected!");

  // Memulai komunikasi dengan OLED pada address 0x3C
  // memberikan pesan jika koneksi gagal
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  // Setup display
  display.display();
  delay(500); // Pause for 2 seconds
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setRotation(0);
}

void loop() {
  // Membaca 50 sample
  for (byte i = 0 ; i < 50 ; i++)
  {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample
    Serial.print(F("red="));
    Serial.print(redBuffer[i], DEC);
    Serial.print(F(", ir="));
    Serial.println(irBuffer[i], DEC);
  }


  // menghitung heart rate and SpO2 setelah 50 samples
  maxim_heart_rate_and_oxygen_saturation(irBuffer, 50, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  // melanjutkan mengambil sample
  while (1)
  {
    // menyimpan 25 set sample pertama ke memory dan 25 terakhir ke atas
    for (byte i = 25; i < 50; i++)
    {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    // mengambil 25 set sample
    for (byte i = 25; i < 50; i++)
    {
      // cek data baru
      while (particleSensor.available() == false) 
        particleSensor.check(); 

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample();
      Serial.print(F("red="));
      Serial.print(redBuffer[i], DEC);
      Serial.print(F(", ir="));
      Serial.print(irBuffer[i], DEC);

      Serial.print(F(", HR="));
      Serial.print(heartRate, DEC);

      Serial.print(F(", HRvalid="));
      Serial.print(validHeartRate, DEC);

      Serial.print(F(", SPO2="));
      Serial.print(spo2, DEC);

      Serial.print(F(", SPO2Valid="));
      Serial.println(validSPO2, DEC);
      
    }

  // Menghitung spo2 dan heartrate final setelah 25 sample baru
  maxim_heart_rate_and_oxygen_saturation(irBuffer, 50, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  // String agar nilai bisa masuk ke json file
  String spo = String(spo2);
  String hr = String(heartRate);

  // json file yang akan dikirim ke database
  String jsonObject = (
    "{\"email\":\"rahmat.wibowo21@gmail.com\",\"spo2\":\""+spo+"\",\"bpm\":\""+hr+"\"}");


  // Reset display
  display.clearDisplay();
  display.setCursor(0, 0);

  display.print("SPO = ");
  display.println(spo);
  display.print("HR = ");
  display.println(hr);
  
  // Mengirim HTTP POST request
  WiFiClient client;
  int retries = 10;
  while(!client.connect(server, 80) && (retries-- > 0)) {
    Serial.print(".");
  }
  Serial.println();
  if(!client.connected()) {
    Serial.println("Failed to connect...");
  } else {
    Serial.println("Client connected!");
  }

  client.println("POST /sensor HTTP/1.1");
  client.println(String("Host: ") + server); 
  client.println("Connection: close\r\nContent-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonObject.length());
  client.println();
  client.println(jsonObject);


  // Timeout
  int timeout = 5 * 10; // 5 seconds             
  while(!client.available() && (timeout-- > 0)){
    delay(100);
  }
  if(!client.available()) {
    Serial.println("No response...");
  }
  while(client.available()){
    Serial.write(client.read());
  }

  // Menutup connection
  Serial.println("\nClosing connection");
  client.stop();

  // Mengirim HTTP POST request setiap 30 detik
  delay(30000);
  display.display();
}
}
