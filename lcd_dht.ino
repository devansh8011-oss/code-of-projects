#include <ESP32Servo.h>

Servo myServo;  // Create a servo object to control the servo
int servoPin = 19; // Define the ESP32 pin connected to the servo signal line

void setup() {
  Serial.begin(115200);

  // Recommended setup for ESP32 timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  myServo.setPeriodHertz(50); // Standard 50Hz servo frequency
  
  // Attach the servo on pin 19. 
  // 500 and 2400 are standard minimum and maximum pulse widths in microseconds.
  myServo.attach(servoPin, 500, 2400); 
  
  // Initialize the servo at 0 degrees
  Serial.println("Moving to 0 degrees");
  myServo.write(0);
  delay(2000); // Wait 2 seconds before starting the loop
}

void loop() {
  // Move to 90 degrees
  Serial.println("Moving to 90 degrees");
  myServo.write(90);
  delay(2000); // Wait 2 seconds
  
  // Return to 0 degrees
  Serial.println("Returning to 0 degrees");
  myServo.write(0);
  delay(2000); // Wait 2 seconds
}