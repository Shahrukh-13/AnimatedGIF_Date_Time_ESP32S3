#include <WiFi.h>
#include <TFT_eSPI.h>
#include <AnimatedGIF.h>
#include <EEPROM.h>
#include "green_liquid.h"


#define GIF_IMAGE green_liquid
#define NORMAL_SPEED 

AnimatedGIF gif;
TFT_eSPI tft = TFT_eSPI();

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -18000;
const int   daylightOffset_sec = 3600;
String Date;
String Time;

bool wifi_flag;

String ssid_i;
String password_i;

String inputMessage = "" ;
String inputParam = "" ;
    
char ssid[100];
char password[100];

String input = "";
String command = "";

static  char                chBuffer[81];

char        chDayOfMonth[3];                                    // Day of month (0 through 31).
char        chDayofWeek[4];                                     // Day of week (Sunday through Saturday).
char        chHour[3];                                          // Hour.
char        chMinute[3];                                        // Minute.
char        chMonth[4];                                         // Month.
const char* chNtpServer = "pool.ntp.org";                       // NTP time server.
char        chSecond[3];                                        // Second.
char        chYear[5];                                          // Year.
char        AM_PM[3];                                           // AM/PM.

uint8_t wifi_connection_timeout_count;

bool DISPLAY_GIF;
void setup() 
{
  Serial.begin(115200);
  EEPROM.begin(512);
  pinMode(15, OUTPUT); // Set pin 15 as output and write it HIGH to boot with external power...
  digitalWrite(15,HIGH);
  pinMode(14,INPUT); // button IO14
   
  wifi_connection_timeout_count = 0;
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  gif.begin(BIG_ENDIAN_PIXELS);
  gif.open((uint8_t *)GIF_IMAGE, sizeof(GIF_IMAGE), GIFDraw);
  tft.startWrite(); 
  tft.setTextDatum(TC_DATUM);

  if(digitalRead(14) == LOW)
  {
    display_menu();
    DISPLAY_GIF = 0;
  }
  else
  {
    DISPLAY_GIF = 1;

    ssid_i = read_String(0);
    password_i = read_String(101);

    ssid_i.toCharArray(ssid, ssid_i.length()+1);
    password_i.toCharArray(password, password_i.length()+1);
    
    //connect to WiFi
    Serial.printf("Connecting to %s ", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED && wifi_connection_timeout_count <10) 
    {
        delay(500);
        Serial.print(".");
        tft.drawString("Connecting", tft.width() / 2, 0, 4);
        wifi_connection_timeout_count++;
    }
  
    if(wifi_connection_timeout_count >=10)
    {
      ESP.restart();
    }
    Serial.println(" CONNECTED");
    tft.fillScreen(TFT_BLACK);
    tft.drawString("Connected", tft.width() / 2, 0, 4);
      
    //init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    GetLocalTime();
  
    //disconnect WiFi as it's no longer needed
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    tft.fillScreen(TFT_BLACK);
  }
}

void loop()
{
  if(DISPLAY_GIF == 0)
  { 
    scan_connect_reset();
  }
  else
  {
    //tft.drawString(Date_Time, 0, 0, 4);;
    tft.drawString(Date, tft.width() / 2, 0, 4);
    tft.drawString(Time, tft.width() / 2, 30, 4);
    gif.playFrame(false, NULL);
    GetLocalTime(); 
  } 
}

void GetLocalTime()
{
  time_t rawtime;
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain date and time");
    Date = "Failed to obtain date";
    Time = "Failed to obtain time";
    ESP.restart();
    //return;
  }
  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  
  //Serial.println(timeinfo.tm_mday);
  /*tm_sec: seconds after the minute;
  tm_min: minutes after the hour;
  tm_hour: hours since midnight;
  tm_mday: day of the month;
  tm_year: years since 1900;
  tm_wday: days since Sunday;
  tm_yday: days since January 1;
  tm_isdst: Daylight Saving Time flag;*/ 

  //time (&rawtime);
  //Date_Time = asctime(localtime (&rawtime));

  strftime(chHour, sizeof(chHour), "%I", & timeinfo);
  strftime(chMinute, sizeof(chMinute), "%M", & timeinfo);
  
  // Then obtain day of week, day of month, month and year.
    
  strftime(chDayofWeek, sizeof(chDayofWeek), "%A", & timeinfo);
  strftime(chDayOfMonth, sizeof(chDayOfMonth), "%d", & timeinfo);
  strftime(chMonth, sizeof(chMonth), "%B", & timeinfo);
  strftime(chYear, sizeof(chYear), "%Y", & timeinfo);
  strftime(AM_PM, sizeof(AM_PM), "%p", & timeinfo);
  
   //sprintf(chBuffer, "%s, %s %s, %s, %s:%s %s", String(chDayofWeek), String(chMonth), String(chDayOfMonth), String(chYear), String(chHour), String(chMinute), String(AM_PM));
   
   sprintf(chBuffer, "%s, %s %s, %s", String(chDayofWeek), String(chMonth), String(chDayOfMonth), String(chYear));
   Date = String(chBuffer);
   sprintf(chBuffer, "%s:%s %s", String(chHour), String(chMinute), String(AM_PM));
   Time = String(chBuffer);
  //Serial.print(Date_Time);
}


void writeString(char add,String data)
{
  int _size = data.length();
  int i;
  for(i=0;i<_size;i++)
  {
    EEPROM.write(add+i,data[i]);
  }
  EEPROM.write(add+_size,'\0');   //Add termination null character for String Data
  EEPROM.commit();
}


String read_String(char add)
{
  int i;
  char data[100]; //Max 100 Bytes
  int len=0;
  unsigned char k;
  k=EEPROM.read(add);
  while(k != '\0' && len<500)   //Read until null character
  {    
    k=EEPROM.read(add+len);
    data[len]=k;
    len++;
  }
  data[len]='\0';
  return String(data);
}

void scan_connect_reset()
{  
  while (Serial.available() > 0)
  {            
    //input += (char) Serial.read(); 
    input =Serial.readString();
    //command =input.substring(0,input.length()-2);  //both NL and CR
    command =input.substring(0,input.length());  // No newline
    if(command == "scan")
    {      
      Serial.println("scan start");
      // WiFi.scanNetworks will return the number of networks found
      int n = WiFi.scanNetworks();
      Serial.println("scan done");
      if (n == 0) 
      {
        Serial.println("no networks found");
      } 
      else 
      {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) 
        {            
          // Print SSID and RSSI for each network found
          Serial.print(i + 1);
          Serial.print(": ");
          Serial.print(WiFi.SSID(i));
          Serial.print(" (");
          Serial.print(WiFi.RSSI(i));
          Serial.print(")");
          Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
          delay(10);
        }
      }
      Serial.println("");
      Serial.print("If you want to connect to a new network, enter 'ssid' and 'passowrd' in following format: ssid,password");
      Serial.println();
    }

    else if(command == "reset")
    {
      ESP.restart();        
    }
    else
    {
      ssid_i = input.substring(0,input.indexOf(','));
      password_i =   input.substring(input.indexOf(',')+1);
  
      writeString(0, ssid_i);
      writeString(101, password_i);
  
      Serial.println();
      Serial.print("new ssid: ");
      Serial.print(ssid_i);
      Serial.print(" , ");
      Serial.print("new password: ");
      Serial.print(password_i);
      Serial.println();
           
      delay(5); 
    }
  }
}

void display_menu()
{
  Serial.println();
  Serial.print("Enter 'scan' to scan for available networks");
  Serial.println();
  Serial.print("If you want to connect to a new network, enter 'ssid' and 'passowrd' in following format: ssid,password");
  Serial.println();
  Serial.print("Enter 'reset' to reset");
  Serial.println();
}
