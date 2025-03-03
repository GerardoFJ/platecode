
// Libraries for WiFi, Secure Client, and Camera functionalities
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <ESP32Servo.h>

#define API_KEY "AIzaSyAuD21YCjefoSrPQc69Z0q1sjjQ6EdHCdE"
#define DATABASE_URL "https://platestation-ecbe9-default-rtdb.firebaseio.com/" 
const char* ssid = "Esp_test";         // Replace "xxx" with your WiFi SSID
const char* password = "esp2025test";      // Replace "xxx" with your WiFi Password
String serverName = "190.168.190.149";  // Replace with your server domain
String serverPath = "/readnumberplate";              // API endpoint path "/readqrcode" "/readnumberplate"
const int serverPort = 5000;                     // HTTPS port
String apiKey = "50A3XYzBNuiA";    
String status = "";         // Replace "xxx" with your API key

#define SERVO_1      14

#define SERVO_STEP   5

Servo servo1;
const int trigPin = 13;
const int echoPin = 15;

int open_c = 130;
int closed = 40;
long duration;
int distance; // GPIO pin for the trigger button

#define flashLight 4      // GPIO pin for the flashlight
int count = 0;           // Counter for image uploads

FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config_fire;
bool signupOK = false;
int count_lol = 0;

WiFiClientSecure client; // Secure client for HTTPS communication

// Camera GPIO pins - adjust based on your ESP32-CAM board
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22


// Function to extract a JSON string value by key
String extractJsonStringValue(const String& jsonString, const String& key) {
  int keyIndex = jsonString.indexOf(key);
  if (keyIndex == -1) {
    return "";
  }

  int startIndex = jsonString.indexOf(':', keyIndex) + 3;
  int endIndex = jsonString.indexOf('"', startIndex);

  if (startIndex == -1 || endIndex == -1) {
    return "";
  }

  return jsonString.substring(startIndex, endIndex);
}


void setup() {
  // Disable brownout detector
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  pinMode(flashLight, OUTPUT);
  digitalWrite(flashLight, LOW);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
 
  servo1.setPeriodHertz(50);    // standard 50 hz servo
  servo1.setPeriodHertz(50);    // standard 50 hz servo
  
  servo1.attach(SERVO_1, 1000, 2000);
  servo1.write(160);
  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());

  // Configure camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Adjust frame size and quality based on PSRAM availability
  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 5;  // Lower number means higher quality (0-63)
    config.fb_count = 2;
    Serial.println("PSRAM found");
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;  // Lower number means higher quality (0-63)
    config.fb_count = 1;
  }

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }
  status = "";

  config_fire.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config_fire.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config_fire, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config_fire.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config_fire.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config_fire, &auth);
  Firebase.reconnectWiFi(true);
  
}

void loop() {
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance= duration*0.034/2;
  Serial.print("Distance: ");
  Serial.println(distance);

  if (distance <= 11  && count_lol >=4) {
   while(status == ""){
     status = sendPhoto();
   }
   if (Firebase.ready() && signupOK ){
    String path = "cars/" + status;
    if(Firebase.RTDB.getInt(&fbdo, path + "/is_in")){
      int is_in = fbdo.to<int>();
      if(is_in != 0){
        Firebase.RTDB.getInt(&fbdo, path + "/entry_time");
        int entryTime = fbdo.to<int>();
        int exitTime = millis() / 1000;
        int duration = exitTime - entryTime;
        Firebase.RTDB.setInt(&fbdo, path + "/exit_time", exitTime);
        Firebase.RTDB.setInt(&fbdo, path + "/last_duration", duration);
        Firebase.RTDB.setInt(&fbdo, path + "/is_in", 0);
        Serial.println("CAR EXIT SUCCESFULL");
      }
      else{
        int entryTime = millis() / 1000;
        Firebase.RTDB.setInt(&fbdo, path + "/entry_time", entryTime);
        Firebase.RTDB.setInt(&fbdo, path + "/is_in", 1);
      }
      servo1.write(40);
      delay(2000);
      servo1.write(160);
    }
    else{
      Serial.println("CAR NOT REGISTERED");
    }

    
  }
  status = "";
  delay(30000);
  count_lol = 0;
  }else if(distance <= 11){
    count_lol++;
  }else{
    count_lol = 0;
  }
  Serial.print("count");
  Serial.println(count_lol);
  delay(50);
}

// Function to capture and send photo to the server
String sendPhoto() {
  String getAll;
  String getBody;

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }
  
  Serial.println("Connecting to server: " + serverName);
  
  client.setInsecure(); //skip certificate validation
  if (client.connect(serverName.c_str(), serverPort)) {
    Serial.println("Connection successful!");    
    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
  
    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    client.println();
    client.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0; n<fbLen; n=n+1024) {
      if (n+1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        client.write(fbBuf, remainder);
      }
    }   
    client.print(tail);
    
    esp_camera_fb_return(fb);
    
    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + timoutTimer) > millis()) {
      Serial.print(".");
      delay(100);      
      while (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (getAll.length()==0) { state=true; }
          getAll = "";
        }
        else if (c != '\r') { getAll += String(c); }
        if (state==true) { getBody += String(c); }
        startTimer = millis();
      }
      if (getBody.length()>0) { break; }
    }
    Serial.println();
    client.stop();
    Serial.println(getBody);
    String NPRData = extractJsonStringValue(getBody, "text");
    Serial.print("NPR");
    Serial.println(NPRData);
    return NPRData;
  }
  else {
    getBody = "Connection to " + serverName +  " failed.";
    Serial.println(getBody);
  }
  Serial.println("finished");
  return "";
}