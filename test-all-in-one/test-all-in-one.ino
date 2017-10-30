/* Name:    Djordje Vujic
   Mail:    djordjevujic@yahoo.com
   github:  djordjevujic
   Title:   Code for testing mouse (animal) activity and logging data into SD card
*/
#include <SPI.h>
#include <SdFat.h>
#include <DS3231.h>

//include serial while debug mode is ON
#define DEBUG 1
#define EVERY_X_MINUTES 1

//SD Card
SdFat SD;
File myFile;

//RTC Clock
DS3231  rtc(SDA, SCL);
Time t;

//Testing existence of minute string in case of power losing
char str[16];     //Must hold one line as a field
size_t n;         //Length of returned field with delimiter

//This string has to be logged via serial port if something went wrong
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
const long interval = 1000;
unsigned long previous_millis = 0;
const int led_pin =  5;
int led_state = LOW;

void setup() {
  // put your setup code here, to run once:
#ifdef DEBUG
  Serial.begin(9600);
#endif
  rtc.begin();

  pinMode(led_pin, OUTPUT);
  //pinMode(button_pin, INPUT_PULLUP); // Enable internal pull-up resistor on pin 5

  attachInterrupt(digitalPinToInterrupt(interrupt_pin), cycle_increment, RISING);

  cli(); //disable interrupts
  sei(); //enable interrupts

  //SD card test
#ifdef DEBUG
  Serial.print("Initializing SD card...");
#endif
  // see if the card is present and can be initialized:
  do
  {
    sd_begin = SD.begin(chipSelect);
    if (sd_begin == false)
    {
      Serial.println("Card failed, or not present");
    }
  } while (sd_begin == false);
  led_problem = false;
  
#ifdef DEBUG
  Serial.println("card initialized.");
#endif
}

void loop()
{ 
  
  // led on pin 5 will blink if a problem occurs

   unsigned long current_millis = millis();
  led_problem = true;
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
  
  digitalWrite(led_pin, led_state);
  Serial.print("Led state: ");
  Serial.println(led_state);
  
  t = rtc.getTime();

  //If day is changed, then make a new log file
  if (t.date != last_day)
  {
    full_date_str = rtc.getDateStr();
    dateStr = dateStringPreparation(full_date_str);

    string_len = dateStr.length() + 1;

    dateStr.toCharArray(file_name, string_len);

    file_exists = SD.exists(file_name);
    Serial.print("File exists: ");
    Serial.println(file_exists);

    myFile = SD.open(file_name, FILE_WRITE);
    if (!myFile)
    {
#ifdef DEBUG
      led_problem = true;
      Serial.println("SD file" + dateStr + " didn't open correctly");
#endif
    }
    else
    {
#ifdef DEBUG
      Serial.println("myFile OPEN OK");
#endif
    }
    if (file_exists == false)
    {
      myFile.println(full_date_str);
      myFile.println("Interval,Cycles");
    }
    myFile.close();

    last_day = t.date;
  }

  //NAPRAVITI: Da se last_min cita iz neke datoteke na SD kartici, ili neko poredjenje i minuta i sata
  if ((t.min % EVERY_X_MINUTES == 0) && (t.min != last_min))
  {
    myFile = SD.open(file_name, FILE_WRITE);
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
        if (n == 0)
        {
          Serial.println("To few lines");
        }
        else
        {
          Serial.print("Size of str: ");
          Serial.println(n);
          Serial.print("Str: ");
          Serial.println(str);
        }
      }
      char *p   = str;
      uint8_t i = 0;
      while (*p)
      {
        if (isdigit(*p) && i < 2)
        {
          day_min_val[i] = strtol(p, &p, 10);
          Serial.print("dayMinVal[");
          Serial.print(i);
          Serial.print("]: ");
          Serial.println(day_min_val[i]);
          i++;
        }
        else
          p++;
      }
    }
    if (t.hour == day_min_val[0] && t.min == day_min_val[1])
    {
      //Serial.println("Hours and minutes are equal!");
      delay(100);
    }
    else
    {
      cli();    //disable interrupt
      
      hour_minute = String(t.hour, DEC);

      myFile.print(hour_minute);
      myFile.print(":");
      myFile.print(String(t.min, DEC));
      myFile.print(",");
      myFile.println(String(num_cycles, DEC));

      Serial.println("Inserted number of cycles: ");
      Serial.print(String(t.hour, DEC));
      Serial.print(":");
      Serial.print(String(t.min, DEC));
      Serial.print(" - ");
      Serial.println(String(num_cycles, DEC));

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
    Serial.print("Number of cycles: ");
    Serial.println(num_cycles);
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

String dateStringPreparation(String str)
{
#ifdef DEBUG
  Serial.println(str);
#endif
  //obrada stringa
  str.remove(2, 1);
  str.remove(4, 1);
  str.remove(4, 1);
  str.remove(4, 1);
#ifdef DEBUG
  Serial.println("Modified string:");
  str += ".csv";
  Serial.println(str);
#endif

  return str;
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
void problem_led_blink()
{
 
}
