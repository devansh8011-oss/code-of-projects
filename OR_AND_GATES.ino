int touch=2;
int ldr=5;
int led=13;
int t;
int l;
void setup() {
  // put your setup code here, to run once:
 pinMode(touch,INPUT);
 Serial.begin(9600);
  pinMode(ldr,INPUT);
  pinMode(led,OUTPUT);
}

void loop() {
  l=digitalRead(ldr);
  t=digitalRead(touch);
  Serial.print("ldr ");
  Serial.println(l);
  delay(1000);
  Serial.print("touch ");
  Serial.println(t);
  delay(1000);
  if(!l==0 || t==0){
    digitalWrite(led,HIGH);
  }
  else{
   digitalWrite(led,LOW);
  }
  Serial.print("ldr ");
  Serial.println(l);

  Serial.print("touch ");
  Serial.println(t);
 
  // put your main code here, to run repeatedly:

}
