#include <Servo.h>
Servo myservo;
void setup() {
  myservo.attach(11);
  pinMode(10,INPUT);
  Serial.begin(9600);
  // put your setup code here, to run once:

}

void loop() {
 
  Serial.println(digitalRead(10));
   if(digitalRead(10)==1){
    myservo.write(90);
    delay(100);
  }
  else if(digitalRead(10)==0){
myservo.write(180);
delay(100);
  }

  // put your main code here, to run repeatedly:

}
