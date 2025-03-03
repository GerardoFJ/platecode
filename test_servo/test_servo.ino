#include <ESP32Servo.h>

// Replace with your network credentials
#define SERVO_1      14

#define SERVO_STEP   5

Servo servo1;
const int trigPin = 13;
const int echoPin = 15;

int open_c = 130;
int closed = 40;
long duration;
int distance;

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
 
  servo1.setPeriodHertz(50);    // standard 50 hz servo
  servo1.setPeriodHertz(50);    // standard 50 hz servo
  
  servo1.attach(SERVO_1, 1000, 2000);
  servo1.write(130);

  
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
  delay(100);
  // servo1.write(40);
}
//40 - 130