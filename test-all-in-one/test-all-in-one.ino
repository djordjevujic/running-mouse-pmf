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
char file_name[30] = "asdsadasdasdasdaf";
int string_len;

//SD chip select
const int chipSelect = 4;

int i;
int i2c_slow_down = 0;
uint8_t last_min;
uint8_t last_day;
int pomDan=0;

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
  
  //making file, starting from this day
  dateStr = rtc.getDateStr();
  dateStr = dateStringPreparation(dateStr);
  
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

  t = rtc.getTime();
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
    dateStr = rtc.getDateStr();
    dateStr = dateStringPreparation(dateStr);
    myFile.close();
    dateStr.toCharArray(file_name, string_len);
    myFile = SD.open(file_name, FILE_WRITE);
    if(!myFile)
    {
        warningString += " SD file" + dateStr + " didn't open correctly ";
        #ifdef DEBUG
          Serial.println("SD file" + dateStr + " didn't open correctly");
        #endif
    }
    last_min = t.min;
    last_day = t.date;
  }
  i2c_slow_down ++;
  delay(1000);  //DELETE LATER
  pomDan++;
  rtc.setDate(pomDan, 10, 2017);

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
