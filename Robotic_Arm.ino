#include <Servo.h>

Servo lservo;
Servo mservo;
Servo hservo;

#define middle A1
#define hand A2
#define lower A3

int mval;
int hval;
int lval;
int l;
int m;
int h;


void setup() {
  lservo.attach(9);
  mservo.attach(12);
  hservo.attach(11); 
  
  pinMode(lower,INPUT);
  pinMode(middle,INPUT);
  pinMode(hand,INPUT);
  Serial.begin(9600);
}

void loop() {
  
  mval=analogRead(middle);
  hval=analogRead(hand);
  lval=analogRead(lower);

  l= map(lval,0,1023,0,180);
  h= map(hval,0,1023,0,180);
  m= map(mval,0,1023,0,180);

  mservo.write(m);
  delay(10);\

  lservo.write(l);
  delay(10);
  
  hservo.write(h);
  delay(10);
  
  Serial.print("lower A3 = ");
  Serial.println(l);
  delay(50);

  Serial.print("middle A1 = ");
  Serial.println(m);
  delay(50);

  Serial.print("hand A2 = ");
  Serial.println(h);
  delay(50);

  // nothing needed — servo holds position
}