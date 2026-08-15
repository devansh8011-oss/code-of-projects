#include <Servo.h>
Servo myservo;
int x = 0;
void setup(){
  myservo.attach(9);
  Serial.begin(9600);

}

void loop (){

  while(x<180){
    myservo.write(x);
    x++;
    delay(10);
    Serial.println(x);
    
  } 
  
  while(x>0){
    myservo.write(x);
    x--;
    delay(10);
    Serial.println(x);
    
  } 
  
}