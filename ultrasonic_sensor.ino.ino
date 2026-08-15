int trig = 9;
int echo = 10;

int distance ;
long duration;
void setup() {
  // put your setup code here, to run once:
Serial.begin(9600);
pinMode(trig, OUTPUT);
pinMode(echo, INPUT);

}

void loop() {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig,HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  duration = pulseIn(echo, HIGH);
  distance = 0.0343 * duration/2;



  // put your main code here, to run repeatedly:
Serial.println(distance);
delay(100);
}
