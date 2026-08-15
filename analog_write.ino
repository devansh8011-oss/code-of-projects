#define ldr A0
float val;
void setup() {
  // put your setup code here, to run once:
 pinMode(ldr,INPUT);
 Serial.begin(9600);
}

void loop() {
  val= analogRead(ldr);
  delay(100);
  Serial.println(val);
  // put your main code here, to run repeatedly:

}
