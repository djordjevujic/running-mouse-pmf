#include <SPI.h>
#include <SD.h>
#include <DS3231.h>

//include serial while debug mode is ON
#define DEBUG 1

DS3231  rtc(SDA, SCL);
File myFile;
Time t;
//This string has to be logged via serial port if something went wrong
String warningString;
String dateStr;
String full_date_str;

char file_name[10] = "asds";
int string_len;

//SD chip select
const int chipSelect = 4;

int i;
int i2c_slow_down = 0;
uint8_t last_min;
uint8_t last_day;
int pomDan=1;
int pomMes=1;

void setup() {
  // put your setup code here, to run once:
  #ifdef DEBUG
    Serial.begin(9600);
  #endif

  rtc.begin();

//SD card test
  #ifdef DEBUG
  Serial.print("Initializing SD card...");
  #endif
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) 
  {
    #ifdef DEBUG
    Serial.println("Card failed, or not present");
    #endif
    // don't do anything more:
    return;
  }
  #ifdef DEBUG
  Serial.println("card initialized.");
  #endif
  /*POSLE OBRISATI*/
  rtc.setDate(pomDan, pomMes, 2017);
  /**************************************/
  //making file, starting from this day
  full_date_str = rtc.getDateStr();
  dateStr = dateStringPreparation(full_date_str);
  
  Serial.print("dateStr: ");
  Serial.println(dateStr);
  
  string_len = dateStr.length() + 1;
  
  Serial.print("string_len: ");
  Serial.println(string_len);
  
  #ifdef DEBUG
  Serial.println("Before toCharArray");
  for(i=0; i<strlen(file_name); i++)
  {
    Serial.print(file_name[i]);
  }
  Serial.println();
  #endif
 
  dateStr.toCharArray(file_name, string_len);
  
  #ifdef DEBUG
  Serial.println();
  Serial.println("After toCharArray");
  for(i=0; i<strlen(file_name); i++)
  {
    Serial.print(file_name[i]);
  }
  Serial.println();
  Serial.print("file_name: ");
  Serial.println(file_name);
  Serial.print("dateStr:");
  Serial.println(dateStr);
  #endif
  
  myFile = SD.open(file_name, FILE_WRITE);
  if(myFile)
  {
    
    #ifdef DEBUG
    myFile.println(full_date_str);
      Serial.println("myFile OPEN OK");
    #endif
    myFile.close();
  }
  else
  {
    #ifdef DEBUG
      Serial.println("Error opening myFile");
    #endif
  }
  
  myFile.close();

  last_min = t.min;
  last_day = t.date;
}

void loop() {

  // put your main code here, to run repeatedly:

  /*
  if(i2c_slow_down == 10)
  {
  */

      t = rtc.getTime();
      i2c_slow_down = 0;
 
  /*    
  }
  */
  //If day is changed, then make a new log file

  if(t.date != last_day) 
  {
    full_date_str = rtc.getDateStr();
    dateStr = dateStringPreparation(full_date_str);
 
    dateStr.toCharArray(file_name, string_len);
    myFile = SD.open(file_name, FILE_WRITE);
    if(!myFile)
    {
        warningString += " SD file" + dateStr + " didn't open correctly ";
        #ifdef DEBUG
          Serial.println("SD file" + dateStr + " didn't open correctly");
        #endif
    }

    myFile.println(full_date_str);
    myFile.close();
    
    last_min = t.min;
    last_day = t.date;
  }
  i2c_slow_down ++;
 // delay(1000);  //DELETE LATER
  pomDan++;
  if(pomDan > 30)
   {
      pomDan = 1;
      pomMes  ++;
      if(pomMes > 12)
        pomMes = 1;
   }
  rtc.setDate(pomDan, pomMes, 2017);

  
  Serial.print("Writing to ");
  Serial.println(full_date_str);
  
  myFile = SD.open(file_name, FILE_WRITE);
  
  myFile.println("1,2,3");
  myFile.println("4,5,6");
  myFile.println("7,8,9");
  
  myFile.close();

  myFile = SD.open(file_name, FILE_WRITE);
  
  myFile.println("10,11,12");
  myFile.println("13,14,15");
  myFile.println("16,17,18");
  
  myFile.close();
  
  delay(2000);
}

String dateStringPreparation(String str)
{
  #ifdef DEBUG
  Serial.println(str);
  #endif
  //obrada stringa
  str.remove(2,1);
  str.remove(4,1);
  str.remove(4,1);
  str.remove(4,1);
  #ifdef DEBUG
  Serial.println("Modified string:");
  str+=".csv";
  Serial.println(str);
  #endif
  
  return str;
}
