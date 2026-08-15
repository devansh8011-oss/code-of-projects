#include <Servo.h>
 
Servo hand;
Servo arm;
int ena = 5;
int in1 = 8;
int in2 = 9;
int in3 = 10;
int in4 = 11;
int enb = 6;
char command;
a[] = {1,2,3,4,5,6,7,8};

void setup() {
  Serial.begin(9600);
  pinMode(ena, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(enb, OUTPUT);
  hand.attach(12);
  arm.attach(13);
  hand.write(120);
  arm.write(180);
  
}

void loop() {
  if (Serial.available()) {
    command = Serial.read();

    if (command == 'F') {
      digitalWrite(in1, HIGH);
      digitalWrite(in2, LOW);
      digitalWrite(in3, HIGH);
      digitalWrite(in4, LOW);
      analogWrite(ena, 200);
      analogWrite(enb, 200);
    }

    else if (command == 'B') {
      digitalWrite(in1, LOW);
      digitalWrite(in2, HIGH);
      digitalWrite(in3, LOW);
      digitalWrite(in4, HIGH);
      analogWrite(ena, 200);
      analogWrite(enb, 200);
    }

   else if (command == 'L') {
      digitalWrite(in1, LOW);
      digitalWrite(in2, HIGH);
      digitalWrite(in3, HIGH);
      digitalWrite(in4, LOW);
      analogWrite(ena, 200);
      analogWrite(enb, 200);
    }

    else if (command == 'R') {
      digitalWrite(in1, HIGH);
      digitalWrite(in2, LOW);
      digitalWrite(in3, LOW);
      digitalWrite(in4, HIGH);
      analogWrite(ena, 200);
      analogWrite(enb, 200);
    }

    else if (command == 'S') {
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      digitalWrite(in3, LOW);
      digitalWrite(in4, LOW);
      analogWrite(ena, 200);
      analogWrite(enb, 200);
    }
    else if (command == 'W') {
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      digitalWrite(in3, LOW);
      digitalWrite(in4, LOW);
      analogWrite(ena, 0);
      analogWrite(enb, 0);
      hand.write(100);
      
    }
    else if (command == 'w') {
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      digitalWrite(in3, LOW);
      digitalWrite(in4, LOW);
      analogWrite(ena, 0);
      analogWrite(enb, 0);
      hand.write(200);
      
    }
    else if (command == 'U') {
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      digitalWrite(in3, LOW);
      digitalWrite(in4, LOW);
      analogWrite(ena, 0);
      analogWrite(enb, 0);
      arm.write(90);
      
    }
    else if (command == 'u') {
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      digitalWrite(in3, LOW);
      digitalWrite(in4, LOW);
      analogWrite(ena, 0);
      analogWrite(enb, 0);
      arm.write(180);
      
    }
    else  {
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      digitalWrite(in3, LOW);
      digitalWrite(in4, LOW);
      analogWrite(ena, 0);
      analogWrite(enb, 0);
      arm.write(180);
      hand.write(180);
      
    }
  }
}