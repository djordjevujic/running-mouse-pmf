

/* Name:    Djordje Vujic
   Mail:    djordjevujic@yahoo.com
   github:  djordjevujic
   Title:   Code for testing mouse (animal) activity and logging data into SD card
*/
#include <SPI.h>
#include <SdFat.h>
#include <DS3231.h>  

//include serial while debug mode is ON
#define DEBUG 6
#define EVERY_X_MINUTES 6
#define RTC_SET_YEAR 'Y'
#define RTC_SET_MONTH 'M'
#define RTC_SET_DAY 'D'
#define RTC_SET_HOUR 'H'
#define RTC_SET_MIN 'm'
#define RTC_SET_SECONDS 's'
#define RTC_GET_DATE 'd'
#define RTC_GET_TIME 't'

#define PRESSED 1
#define RELEASED 0

//SD Card
SdFat SD;
File myFile;

//RTC Clock
DS3231  rtc(SDA, SCL);
Time t;
Time temp_time;

//Testing existence of minute string in case of power losing

//@todo -> check if size of this string is OKAY
char line_str[20];     //Must hold one line as a field
size_t n;         //Length of returned field with delimiter

//This string has to be logged via serial port if something went wrong
char file_name[13] = "data_log.csv";
uint8_t string_len;
bool file_exists = false; //if file does not exist, then write date,
                          // time and cycles sections at begin of sheet
char uart_income_byte = 0;

//buttons
const int btnPause = 6; //on uno: 6
const int btnContinue = 7; //on uno: 7
uint8_t btn_pause_read = RELEASED;
uint8_t btn_cont_read = RELEASED;
uint8_t last_btn_pause_state = RELEASED;
uint8_t last_btn_cont_state = RELEASED;
uint8_t btn_pause_state = RELEASED;
uint8_t btn_cont_state = RELEASED;
boolean btn_state_changed = false;

boolean pause_flag = false;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

//leds
uint8_t led_pin_pause = 8;
uint8_t led_pin_cont = 9;

//SD chip select pin
const int chipSelect = 10; //on uno: 4
bool sd_begin = false;

uint8_t last_min = -1;
uint8_t last_day = 0;
//@todo Check size of this array
uint8_t time_val[6];

//interrupt variables
const byte interrupt_pin = 3; //on uno: 2
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

#ifdef DEBUG
  Serial.begin(9600);
#endif

  rtc.begin();
  //sertc.setDate(26, 9, 2018);
  pinMode(led_pin, OUTPUT);
  pinMode(btnPause, INPUT);
  pinMode(btnContinue, INPUT);
  
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
      //led_problem_blink();
      digitalWrite(led_pin_pause, HIGH);
      delay(900);
      digitalWrite(led_pin_pause, LOW);
  //    delay(600);
    }
  } while (sd_begin == false);
  led_problem = false;
  
#ifdef DEBUG
  Serial.println("card initialized.");
#endif
  if(pause_flag == false)
    digitalWrite(led_pin_cont, HIGH);

    //rtc.setDate(7, 10, 2018);
    temp_time = rtc.getTime();

    temp_time.min = 0;
    temp_time.hour = 0;
    temp_time.sec = 0;

    //rtc.setTime(temp_time.hour, temp_time.min, temp_time.sec);
    rtc.setTime(0, 0, 0);
}

void loop()
{
  // Setting temporary date and time
  
  #ifdef DEBUG
    uart_read_msg();
  #endif

  btn_pause_read = digitalRead(btnPause);
  btn_cont_read = digitalRead(btnContinue);
  
  //debouncing
  if(btn_pause_read != last_btn_pause_state)
  {
    lastDebounceTime = millis();
    btn_state_changed = true;
  }

  if((millis() - lastDebounceTime) > debounceDelay && btn_state_changed == true)
  {
    if(btn_pause_read != btn_pause_state)
    {
      btn_pause_state = btn_pause_read;
      if(btn_pause_state == PRESSED)
      {
        pause_flag = true;
        digitalWrite(led_pin_pause, HIGH);
        digitalWrite(led_pin_cont, LOW);
        #ifdef DEBUG
        Serial.println("Pause PRESSED!");
        #endif
      }
      if(btn_pause_state == RELEASED)
      {
        #ifdef DEBUG
        Serial.println("Pause RELEASED!");
        #endif
      }
      btn_state_changed = false;
    }
  }
  last_btn_pause_state = btn_pause_read;


  if(btn_cont_read != last_btn_cont_state)
  {
    lastDebounceTime = millis();
    btn_state_changed = true;
  }

  if((millis() - lastDebounceTime) > debounceDelay && btn_state_changed == true)
  {
    if(btn_cont_read != btn_cont_state)
    {
      btn_cont_state = btn_cont_read;
      if(btn_cont_state == PRESSED)
      {
        pause_flag = false;
        digitalWrite(led_pin_pause, LOW);
        digitalWrite(led_pin_cont, HIGH);
        #ifdef DEBUG
        Serial.println("cont PRESSED!");
        Serial.print("Pause_flag: ");
        Serial.println(pause_flag);
        #endif
      }
      if(btn_cont_state == RELEASED)
      {
        #ifdef DEBUG
        Serial.println("cont RELEASED!");
        #endif
      }
      btn_state_changed = false;
    }
  }
  last_btn_cont_state = btn_cont_read;

  //MAIN PART OF CODE
  if(pause_flag == true)
  {
    myFile.close();
  }
  else
  {   
    
      rtc.setTime(temp_time.hour, temp_time.min, temp_time.sec);
      Serial.print("Time str: ");
      Serial.println(rtc.getTimeStr());
      
      rtc.setDate(temp_time.date, temp_time.mon, temp_time.year);
      Serial.print("Date str: ");
      Serial.println(rtc.getDateStr());
    
      
      t = rtc.getTime();

      // Setting temporary date and time for testing
      temp_time.min++;
      //Serial.print("Min: ");
      //Serial.println(temp_time.min);
      
      if(temp_time.min > 59)
      {
        temp_time.min = 0;
        temp_time.hour++;
        Serial.print("Hour: ");
        Serial.println(temp_time.hour);
      }
      
      if(temp_time.hour > 23)
      {
        temp_time.hour = 0;
        temp_time.date++;
        Serial.print("Date: ");
        Serial.println(temp_time.date);
      }
      
      
      temp_time.sec = 0;
  
      // led on pin 5 will blink if a problem occurs
      //led_problem_blink();
      //digitalWrite(led_pin, led_state);
      
      /*
      file_exists = SD.exists(file_name);
      Serial.print("File exists: ");
      Serial.println(file_exists);
      */
      myFile = SD.open(file_name, FILE_WRITE);
      if (!myFile)
      {
        led_problem = true;
        #ifdef DEBUG     
        //Serial.println("SD file" + file_name + " didn't open correctly");
        #endif
      }
      
      else
      {
        #ifdef DEBUG
        //Serial.println("myFile OPEN OK");
        led_problem = false;
        #endif
      }

      myFile.close();
  
      last_day = t.date;

      if (first_power_on == true)
      {
        file_exists = SD.exists(file_name);
        Serial.print("File exists: ");
        Serial.println(file_exists);
    
        myFile = SD.open(file_name, FILE_WRITE);
        if (!myFile)
        {
          led_problem = true;
          #ifdef DEBUG     
          //Serial.println("SD file" + file_name + " didn't open correctly");
          #endif
        }
        else
        {
          #ifdef DEBUG
          Serial.println("myFile OPEN OK");
          led_problem = false;
          #endif
        }
        
        first_power_on = false;
        myFile.rewind();

        //@todo -> this takes much time when file is long
        // Good option for debugging
        // Shows if file has too many lines!
        /*
        while (myFile.available() > 0)
        {
          n = readField(&myFile, line_str, sizeof(line_str), "\n");
          if (n == 0)
          {
            Serial.println("To few lines");
          }
          else
          {
            //Serial.print("Size of line_str: ");
            //Serial.println(n);
            Serial.print("line_str: ");
            Serial.println(line_str);
          }
        }
        */
        /*
        char *p   = line_str;
        uint8_t i = 0;

        //@todo -> delete everything with time_val
        
        while (*p)
        {
          if (isdigit(*p) && i < 6)
          {
            time_val[i] = strtol(p, &p, 10);
            Serial.print("timeVal[");
            Serial.print(i);
            Serial.print("]: ");
            Serial.println(time_val[i]);
            i++;
          }
          else
            p++;
        }
        */
      }
      
      //NAPRAVITI: Da se last_min cita iz neke datoteke na SD kartici, ili neko poredjenje i minuta i sata
      //@todo Added t.sec == 0 -> test it
      
      
      if ((t.min % EVERY_X_MINUTES == 0) && (t.sec == 0) && (t.min != last_min))
      {
    
        myFile = SD.open(file_name, FILE_WRITE);
        if(!myFile)
          led_problem = true;
        else
          led_problem = false;
        //This condition is for case of power losing.
        //Then, Arduino must first check logfile, and in case that there is not data
        //logged with the same hours and minutes, log the newest data.
        /*
        if (t.hour == time_val[0] && t.min == time_val[1])
        {
          //Serial.println("Hours and minutes are equal!");
          //delay(100);
        }
        */
//        else
//        {
          cli();    //disable interrupt

          myFile.print(String(t.year));
          myFile.print("-");
          
          if(t.mon < 10)
            myFile.print("0");
          myFile.print(String(t.mon));
          
          myFile.print("-");

          if(t.date < 10)
            myFile.print("0");
          myFile.print(String(t.date));
          
          myFile.print(" ");

          if(t.hour < 10)
            myFile.print("0");
          myFile.print(String(t.hour));
          
          myFile.print(":");

          if(t.min < 10)
            myFile.print("0");
          myFile.print(String(t.min));
          
          myFile.print(":");

          if(t.sec < 10)
            myFile.print("0");
          myFile.print(String(t.sec));

          myFile.print(",");
          
          myFile.println(String(num_cycles, DEC));

          
          #ifdef DEBUG
          
          Serial.print(String(t.year));
          Serial.print("-");
          Serial.print(String(t.mon));
          Serial.print("-");
          Serial.print(String(t.date));
          Serial.print(" ");
          Serial.print(String(t.hour));
          Serial.print(":");
          Serial.print(String(t.min));
          Serial.print(":");
          Serial.print(String(t.sec));
      
          Serial.print(",");
      
          Serial.println(String(num_cycles, DEC));
          #endif
    
          num_cycles = 0;
          sei();   //enable interrupt
          last_min = t.min;
          myFile.close();
 //       }
      }
  }
  //ako je sisnuto pause, zatvori datoteku
  
   
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
  //Serial.print("led_problem: ");
  //Serial.println(led_problem);
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

#ifdef DEBUG
//reads uart message, and sets date and time if user wants
//test function
  void uart_read_msg()
  {
    if (Serial.available() > 0)
    {
      uart_income_byte = Serial.read();
      
      //Serial.print("Byte: ");
      //Serial.println(uart_income_byte, DEC);
    
      switch(uart_income_byte)
      {
        case RTC_GET_DATE:
        {
          Serial.println(rtc.getDateStr());
          
          break;
        }

        case RTC_GET_TIME:
        {
          Serial.println(rtc.getTimeStr());
          
          break;
        }
        
        case RTC_SET_YEAR:
        {
           uint8_t nums[4];
           int year=0;
           int koef = 1000;
           int i=0;

           Serial.println("RTC SET YEAR");
           
           for(i=0; i<4; i++)
           {
              while(Serial.available() == 0);
              nums[i] = Serial.read() - 48;
              year += nums[i] * koef;
              koef /= 10;
              //Serial.print("Koef: ");
              //Serial.println(koef);
              //Serial.print("Year");
              //Serial.println(year);
           }
           t = rtc.getTime();
           t.year = year;

           rtc.setDate(t.date, t.mon, t.year);
           
           break;
        }
 
 
        // 'D'
        case RTC_SET_DAY:
        {
          Serial.println("RTC_SET_DAY");
          
          uint8_t inputBig = 0;
          uint8_t inputSmall = 0;
          
          Serial.println("RTC SET DAY");
          while(Serial.available() == 0);
          inputBig = Serial.read() - 48;

          while(Serial.available() == 0);
          inputSmall = Serial.read() - 48;

          t = rtc.getTime();
          t.date = inputBig*10 + inputSmall;
          rtc.setDate(t.date, t.mon, t.year);
          Serial.println(rtc.getDateStr());
          
          break;
        }

        // 'M'
        case RTC_SET_MONTH:
        {
          uint8_t inputBig = 0;
          uint8_t inputSmall = 0;
          
          Serial.println("RTC SET MONTH");
          while(Serial.available() == 0);
          inputBig = Serial.read() - 48;
          Serial.println(inputBig);
          
          while(Serial.available() == 0);
          inputSmall = Serial.read() - 48;
          Serial.println(inputSmall);

          t = rtc.getTime();
          t.mon = inputBig*10 + inputSmall;
          rtc.setDate(t.date, t.mon, t.year);
          Serial.println(rtc.getDateStr());
          
          break;
        }

        // 'H'
        case RTC_SET_HOUR:
        {
          uint8_t hourBig = 0;
          uint8_t hourSmall = 0;
          
          Serial.println("RTC SET HOUR");
          while(Serial.available() == 0);
          hourBig = Serial.read() - 48;

          while(Serial.available() == 0);
          hourSmall = Serial.read() - 48;

          t = rtc.getTime();
          t.hour = hourBig*10 + hourSmall;
          rtc.setTime(t.hour, t.min, t.sec);
          Serial.println(rtc.getTimeStr());
          
          break;
        }

        // 'm'
        case RTC_SET_MIN:
        {
          uint8_t minutesBig = 0;
          uint8_t minutesSmall = 0;
          
          Serial.println("RTC SET MINUTES");
          while(Serial.available() == 0);
          minutesBig = Serial.read() - 48;

          while(Serial.available() == 0);
          minutesSmall = Serial.read() - 48;

          t = rtc.getTime();
          t.min = minutesBig*10 + minutesSmall;
          rtc.setTime(t.hour, t.min, t.sec);
          Serial.println(rtc.getTimeStr());
          
          break;
        }

        // 's'
        case RTC_SET_SECONDS:
        {
          uint8_t inputBig = 0;
          uint8_t inputSmall = 0;
          
          Serial.println("RTC SET SECONDS");
          while(Serial.available() == 0);
          inputBig = Serial.read() - 48;

          while(Serial.available() == 0);
          inputSmall = Serial.read() - 48;

          t = rtc.getTime();
          t.sec = inputBig*10 + inputSmall;
          rtc.setTime(t.hour, t.min, t.sec);
          Serial.println(rtc.getTimeStr());
          
          break;
        }

        //Enter
        case 10:
        {
          break;
        }
        
        default:
        {
          Serial.println("Unknown message (uart_read_msg())");
          break;
        }
      }
    }
  }
#endif
