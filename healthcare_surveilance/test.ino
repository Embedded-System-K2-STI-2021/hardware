#include <SimpleTimer.h>
#include <WiFi.h>

char ssid[] = "---------"; //Enter WIFI Name
char pass[] = "---------"; //Enter WIFI Password
 
SimpleTimer timer;
 
int mq2 = A0; // smoke sensor is connected with the analog pin A0 
int data = 0; 
// Domain name serta URL server database
const char* server = "reksti-k2-t3.herokuapp.com";
void setup() 
{
  Serial.begin(115200);
   WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("WiFi connected!");
  timer.setInterval(1000L, getSendData);
}
 
void loop() 
{
  timer.run(); // Initiates SimpleTimer
}
 
void getSendData()
{
data = analogRead(mq2); 
  String ppm = String(data);

  // json file yang akan dikirim ke database
  String jsonObject = (
    "{\"email\":\"rahmat.wibowo21@gmail.com\",\"ppm\":"+ppm+"}");
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
 
}
