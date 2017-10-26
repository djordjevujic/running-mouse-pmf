//Implementirati:
/*  Ako se u nekom slucaju Arduino iskljuci, i nakon toga ukljuci, a ne uspe ocitati SD kartica u Setup-u, while petlju koja traje sve dok se kartica ne ucita
 *  Napraviti posebnu datoteku gde se upisuju vec upisane minutaze, a pre svakog upisa trenutna minutaza poredi sa tom iz datoteke
*/

#include <SPI.h>
#include <SD.h>
#include <DS3231.h>

//include serial while debug mode is ON
#define DEBUG 1
#define EVERY_X_MINUTES 1

DS3231  rtc(SDA, SCL);
File myFile;
Time t;

//This string has to be logged via serial port if something went wrong
String warningString;
String dateStr;
String full_date_str;
char file_name[10] = "asds";
uint8_t string_len;
bool file_exists = false;

//SD chip select
const int chipSelect = 4;
bool sd_begin = false;

uint8_t last_min = 0;
uint8_t last_day = 0;
uint8_t pomDan = 1;
uint8_t pomMes = 1;

//interrupt variables
const byte interrupt_pin = 2;
volatile int num_cycles = 0;
int result;
int result_old;
bool kp = false;

void setup() {
  // put your setup code here, to run once:
#ifdef DEBUG
  Serial.begin(9600);
#endif

  rtc.begin();

  attachInterrupt(digitalPinToInterrupt(interrupt_pin), cycle_increment, RISING);

  cli(); //disable interrupts
  sei(); //enable interrupts

  //SD card test
#ifdef DEBUG
  Serial.print("Initializing SD card...");
#endif
  // see if the card is present and can be initialized:
 /*
  if (!SD.begin(chipSelect))
  {
#ifdef DEBUG
    Serial.println("Card failed, or not present");
#endif
    // don't do anything more:
    return;
  }
*/
  do
  {
      sd_begin = SD.begin(chipSelect);
      if(sd_begin == false)
      {
        Serial.println("Card failed, or not present");
        delay(1000);
      }
   }while(sd_begin == false);
#ifdef DEBUG
  Serial.println("card initialized.");
#endif
  /*POSLE OBRISATI*/
 // rtc.setDate(26, 10, 2017);
  /**************************************/
}

void loop() {

  // put your main code here, to run repeatedly:
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
      warningString += " SD file" + dateStr + " didn't open correctly ";
#ifdef DEBUG
      Serial.println("SD file" + dateStr + " didn't open correctly");
#endif
    }
    else
    {
#ifdef DEBUG
      Serial.println("myFile OPEN OK");
#endif
    }
    if(file_exists == false)
    {
      myFile.println(full_date_str);
      myFile.println("Interval,Cycles");
    }
    myFile.close();

    last_day = t.date;
  }
  /*
  pomDan++;
  if (pomDan > 30)
  {
    pomDan = 1;
    pomMes  ++;
    if (pomMes > 12)
      pomMes = 1;
  }
  */

  //rtc.setDate(pomDan, pomMes, 2017);
  if ((t.min % EVERY_X_MINUTES == 0) && (t.min != last_min))
  {
    myFile = SD.open(file_name, FILE_WRITE);

    cli();    //disable interrupt

    myFile.print(String(t.hour, DEC));
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
  cli();
  result_old = result;
  result = num_cycles;
  sei();
  if (result_old != result)
    {
      Serial.print("Number of cycles: ");
      Serial.println(num_cycles);
    }
  // delay(2000);
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
    //last_interrupt_time = interrupt_time;
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
