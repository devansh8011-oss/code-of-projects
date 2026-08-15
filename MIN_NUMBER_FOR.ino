int list[]={1,-2,4,2,-9,8};
int min=0;
void setup() {
  Serial.begin(9600);
  for(int i=0;i<6;i++){
    if(list[i]<min){
      min=list[i];
    }
  }
  // put your setup code here, to run once:
 Serial.println(min);
}

void loop() {
  // put your main code here, to run repeatedly:

}
