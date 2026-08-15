#include <Servo.h>
Servo servo;
#define x A0
#define y A1
int xval;
int yval;
int X;
int Y;

void setup() {servo.attach(9);
 pinMode(x,INPUT);
 pinMode(Y,INPUT);
 Serial.begin(9600);

  // put your setup code here, to run once:

}

void loop() {
  xval=analogRead(x);
  yval=analogRead(y);
  X = map(xval, 0,1023,0,180);
  servo.write(X);
  

  // put your main code here, to run repeatedly:

}
