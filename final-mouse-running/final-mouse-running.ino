/* Name:    Djordje Vujic
   Mail:    djordjevujic@yahoo.com
   github:  djordjevujic
   Title:   Code for testing mouse (animal) activity and logging data into SD card
*/
#include <SPI.h>
#include <SdFat.h>
#include <DS3231.h>

#define EVERY_X_MINUTES 1

void led_problem_blink();
String dateStringPreparation(String str_of_date);
size_t readField(File* file, char* str, size_t size, const char* delim);



//SD Card
SdFat SD;
File myFile;

//RTC Clock
DS3231  rtc(SDA, SCL);
Time t;

//Testing existence of minute string in case of power losing
char str[16];     //Must hold one line as a field
size_t n;         //Length of returned field with delimiter

String dateStr;
String full_date_str;
String hour_minute;
char file_name[10] = "start";
uint8_t string_len;
bool file_exists = false; //if file does not exist, then write date,
                          // time and cycles sections at begin of sheet

//SD chip select pin
const int chipSelect = 4;
bool sd_begin = false;

uint8_t last_min = 0;
uint8_t last_day = 0;
uint8_t day_min_val[2];

//interrupt variables
const byte interrupt_pin = 2;
volatile int num_cycles  = 0;
int result;
int result_old;
bool kp = true;

bool first_power_on  = true;
bool led_problem     = true;
const long interval = 500;
unsigned long previous_millis = 0;
const int led_pin =  5;
int led_state = LOW;

void setup() {
  
  Serial.begin(9600);
  
  rtc.begin();

  pinMode(led_pin, OUTPUT);
  //pinMode(button_pin, INPUT_PULLUP); // Enable internal pull-up resistor on pin 5
  attachInterrupt(digitalPinToInterrupt(interrupt_pin), cycle_increment, RISING);

  cli(); //disable interrupts
  sei(); //enable interrupts

  //SD card test

  // see if the card is present and can be initialized:
  do
  {
    sd_begin = SD.begin(chipSelect);
    if (sd_begin == false)
    {
      digitalWrite(led_pin, HIGH);
      delay(500);
      digitalWrite(led_pin, LOW);
      delay(500);
    }
  } while (sd_begin == false);
  led_problem = false;
}

void loop()
{ 
  
  // led on pin 5 will blink if a problem occurs
  led_problem_blink();
  digitalWrite(led_pin, led_state);
  
  t = rtc.getTime();

  //If day is changed, then make a new log file
  if (t.date != last_day)
  {
    full_date_str = rtc.getDateStr();
    dateStr = dateStringPreparation(full_date_str);

    string_len = dateStr.length() + 1;
    dateStr.toCharArray(file_name, string_len);
    file_exists = SD.exists(file_name);

    myFile = SD.open(file_name, FILE_WRITE);
    if (!myFile)
    {
      led_problem = true;
    }
    else
    {
      led_problem = false;
    }
    if (file_exists == false)
    {
      myFile.println(full_date_str);
      myFile.println("Interval,Cycles");
    }
    myFile.close();
    last_day = t.date;
  }
  
  if ((t.min % EVERY_X_MINUTES == 0) && (t.min != last_min))
  {
    myFile = SD.open(file_name, FILE_WRITE);
    if(!myFile)
      led_problem = true;
    else
      led_problem = false;
    //This condition is for case of power losing.
    //Then, Arduino must first check logfile, and in case that there is not data
    //logged with the same hours and minutes, log the newest data.
    if (first_power_on == true)
    {
      first_power_on = false;
      myFile.rewind();
      while (myFile.available() > 0)
      {
        n = readField(&myFile, str, sizeof(str), "\n");
      }
      char *p   = str;
      uint8_t i = 0;
      while (*p)
      {
        if (isdigit(*p) && i < 2)
        {
          day_min_val[i] = strtol(p, &p, 10);
          i++;
        }
        else
          p++;
      }
    }
    if (t.hour == day_min_val[0] && t.min == day_min_val[1])
    {
      delay(100);
    }
    else
    {
      cli();    //disable interrupt

      myFile.print(String(t.hour, DEC));
      myFile.print(":");
      myFile.print(String(t.min, DEC));
      myFile.print(",");
      myFile.println(String(num_cycles, DEC));

      num_cycles = 0;
      sei();   //enable interrupt
      last_min = t.min;
      myFile.close();
    }
  }
  //-------------------------------------------
  //Posle vratiti u if petlju minuta i optimizovati tu petlju (cli, sei) --> cli predugo zadrzava
  
  cli();
  result_old = result;
  result = num_cycles;
  sei();
  if (result_old != result)
  {
    
  }
  //-----------------------------------------------
}

//Interrupt function
void cycle_increment()
{
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();

  if (kp == false)
  {
    last_interrupt_time = interrupt_time;
    kp = true;
  }

  if (digitalRead(interrupt_pin) && (kp == true) && (interrupt_time - last_interrupt_time > 200))
  {
    num_cycles++;
    kp = false;
  }
}

String dateStringPreparation(String str_of_date)
{
  //obrada stringa
  str_of_date.remove(2, 1);
  str_of_date.remove(4, 1);
  str_of_date.remove(4, 1);
  str_of_date.remove(4, 1);

  str_of_date += ".csv";

  return str_of_date;
}

//Function for reading one field from CSV file
size_t readField(File* file, char* str, size_t size, const char* delim) {
  char ch;
  size_t n = 0;
  while ((n + 1) < size && file->read(&ch, 1) == 1) {
    // Delete CR.
    if (ch == '\r') {
      continue;
    }
    str[n++] = ch;
    if (strchr(delim, ch)) {
      break;
    }
  }
  str[n] = '\0';
  return n;
}
void led_problem_blink()
{
  unsigned long current_millis = millis();

  if(led_problem == true && (current_millis - previous_millis >= interval))
  {
    previous_millis = current_millis;
    if(led_state == LOW)
    {
      led_state = HIGH;
    }
    else
    {
      led_state = LOW;
    }
  }
  else if(led_problem == false)
  {
    led_state = LOW;  
  }
}
