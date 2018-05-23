/*
 * PIR sensor tester
 */
 
int ledPin = 5;                  // choose the pin for the LED
int inputPin = 12;               // choose the input pin (for PIR sensor)
int pirState = LOW;              // we start, assuming no motion detected
 
void setup() {
  pinMode(ledPin, OUTPUT);      // declare LED as output
  pinMode(inputPin, INPUT);     // declare sensor as input
 
  Serial.begin(115200);
}
 
void loop(){
  if (digitalRead(inputPin) == HIGH) {            // check if the input is HIGH
    digitalWrite(ledPin, LOW);  // turn LED ON
    if (pirState == LOW) {
      // we have just turned on
      Serial.println("Motion detected!");
      // We only want to print on the output change, not state
      pirState = HIGH;
      Serial.println("restarting in 10 sec...");
      delay(10000);
      ESP.restart();
      
    }
  } else {
    digitalWrite(ledPin, HIGH); // turn LED OFF
    if (pirState == HIGH){
      // we have just turned of
      Serial.println("Motion ended!");
      // We only want to print on the output change, not state
      pirState = LOW;
    }
  }
}
