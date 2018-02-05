TaskHandle_t Task1;
long loops1 = 5000;
long loops2 = 5000; 
long q = 0;

void artificalLoadForBenchmarking()
{
  double t3; 
  long t1, t2;

  for(int i = 0; i < loops1; ++i)
  {
    for(int j = 1; j < loops2; ++j)
    {
        q++;
        t1 = 5000 * i;
        t2 = 125 * 345 * i;
        t3 = 345 %j;      
    }
  }
}

void codeForTask1(void* param)
{ 
  while(true)
  {  
    // loop here
    Serial.print("codeForTask1 :: runs on Core: ");
    Serial.println(xPortGetCoreID());

    long start = millis();
    artificalLoadForBenchmarking();
    Serial.print("Time : ");
    Serial.println(millis() - start);
  }
}
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  xTaskCreatePinnedToCore(
    codeForTask1,   // code to run
    "Task 1",       // name of task
    1000,           // stack size
    NULL,           // parameters to pass to task
    1,              // priority
    &Task1,         // code to run
    0);             // Core to run on : choose between 0 and 1 (1 is default for ESP32 IDE)
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("loop: runs on Core: ");
  Serial.println(xPortGetCoreID());
  long start2 = millis();
  artificalLoadForBenchmarking();
  Serial.print("Time : ");
  Serial.println(millis() - start2);
  
}
