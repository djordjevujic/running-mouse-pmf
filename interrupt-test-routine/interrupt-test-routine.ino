//interrupt test routine
const byte interrupt_pin = 2;
volatile int num_cycles = 0;
int result;
int result_old;
bool kp = false;

void setup() {
  Serial.begin(9600);
  Serial.println("Program started"); 
  
  //pinMode(interrupt_pin, INPUT);

  attachInterrupt(digitalPinToInterrupt(interrupt_pin),cycle_increment, RISING);

  cli(); //disable interrupts
  sei(); //enable interrupts
}

void loop() {
  cli();
  result_old = result;
  result = num_cycles;
  sei();
  if(result_old != result)
  {
  Serial.print("Number of cycles: ");
  Serial.println(num_cycles);
  } 
}

void cycle_increment()
{
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();
       
       if(kp == false)
       {
           last_interrupt_time = interrupt_time;
           kp = true;
       }
    
    if(digitalRead(interrupt_pin) && (kp == true) && (interrupt_time - last_interrupt_time > 200))
    {
        num_cycles++;
        kp = false;
        //last_interrupt_time = interrupt_time;
    }
}

