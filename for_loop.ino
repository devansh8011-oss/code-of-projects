#include <Servo.h>
Servo servo;
int degrees[]={50,100,150};
void setup() {
  servo.attach(9);

  // put your setup code here, to run once:

}

void loop() {
  for(int i=0;i<=3;i++){
    servo.write(degrees[i]);
    delay(1000);
  }
  // put your main code here, to run repeatedly:

}
