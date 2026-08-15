
void setup() {
  Serial.begin(9600);
  pinMode(pin2,INPUT);
  pinMode(pin,OUTPUT)

  // put your setup Serial.begin(9600);
}

void loop() {
  digitalWrite(4, checksensor(2));
  blinkled(10);

  // put your main code here, to run repeatedly:

}
void blinkled(int pin){
  digitalWrite(pin,HIGH);
  delay(1000);
  digitalWrite(pin,LOW);
  delay(13,LOW);
}
int checksensor(int pin2){
  int sensor = digitalRead(pin2);
  if (sensor == 0){
    return 1;
  }
  else{
    return 0;
  }
}

}
