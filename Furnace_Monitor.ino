/*
     Furnace Run time      "Furnace_Monitor.ino"  09/13/2018 

     Developed for RobotDyn WiFi D1 R2 board with SPIFFS

     Sketch --developement started by William M. Lucid, AB9NQ    08/01/2018 @ 19:29 EDT

     ThingSpeak is disabled during testing phase!!!

*/

#include <ESP8266WiFi.h>   //Part of ESP8266 Board Manager install
#include <sys/time.h>                   // struct timeval
#include <time.h>
#include<FS.h>
#include <ThingSpeak.h>   //https://github.com/mathworks/thingspeak-arduino . Get it using the Library Manager

extern "C"
{
#include "user_interface.h"
}


// Replace with your network details
const char* ssid = "yourssid";
const char* password = "yourpassword";

//Used with NTP Time Protocol
#define NTP0 "us.pool.ntp.org"
#define NTP1 "time.nist.gov"
#define TZ "EST+5EDT,M3.2.0/2,M11.1.0/2"

//Used to detect Furnance running (off = HIGH/on = LOW)
#define furnace D6
volatile int dayFlag = 0;
unsigned long int start, finished, elapsed, minutes;

int value;

#define online D7  //pin for online LED indicator

int DOW, MONTH, DATE, YEAR, HOUR, MINUTE, SECOND;   //Used with NTP Time protocol and struct tm *ti;.

char strftime_buf[64];

String dtStamp(strftime_buf);   //Used for Date Time Stamp.
String lastUpdate;   //Hold last update Date Time Stamp.


//Used with struct tm *ti
int lc = 0;
time_t tnow = 0;

char *filename;
char str[] = {0};

String fileRead;

char MyBuffer[13];

String publicIP = "xxx.xxx.xxx.xxx";   //in-place of xxx.xxx.xxx.xxx put your Public IP address inside quotes

#define LISTEN_PORT           yyyy // in-place of yyyy put your listening port number
// The HTTP protocol uses port 80 by default.

#define MAX_ACTION            10      // Maximum length of the HTTP action that can be parsed.

#define MAX_PATH              64      // Maximum length of the HTTP request path that can be parsed.
// There isn't much memory available so keep this short!

#define BUFFER_SIZE           MAX_ACTION + MAX_PATH + 20  // Size of buffer for incoming request data.
// Since only the first line is parsed this
// needs to be as large as the maximum action
// and path plus a little for whitespace and
// HTTP version.

#define TIMEOUT_MS           500   // Amount of time in milliseconds to wait for     /////////default 500/////
// an incoming request to finish.  Don't set this
// too high or your server could be slow to respond.

uint8_t buffer[BUFFER_SIZE + 1];
int bufindex = 0;
char action[MAX_ACTION + 1];
char path[MAX_PATH + 1];

//////////////////////////
// Web Server on port LISTEN_PORT
/////////////////////////
WiFiServer server(LISTEN_PORT);
WiFiClient client;

int error = 0;

int count = 0;
int countYear;

int days;

/*
     This is the ThingSpeak channel number for the MathwWorks weather station
     https://thingspeak.com/channels/YourChannelNumber.  It senses a number of things and puts them in the eight
     field of the channel:

     Field 1 - Temperature (Degrees F)
     Field 2 - Humidity (%RH)
     Field 3 - Barometric Pressure (in Hg)
     Field 4 - Dew Point  (Degree F)
*/

//edit ThingSpeak.com data here...
unsigned long myChannelNumber = yourchannelnumber;
const char * myWriteAPIKey = "yourkey";

float totalDay = 0;   //elapsed time in milliseconds for day.
float totalMonth = 0;   //elasped time in milliseconds for month.
float monthTotal = 0;
float totalYear = 0;   //elapsed time in milliseconds for year

//int flag = 0;

String tFD;
String tFY;

String Month;

////////////
void setup()
{

     Serial.begin(9600);

     while (!Serial) {}

     Serial.println("");
     Serial.println("");
     Serial.println("Starting...");
     Serial.print("testimg.ino");
     Serial.print("\n");

     pinMode (furnace, INPUT_PULLUP);

     attachInterrupt (digitalPinToInterrupt (furnace), runTime, CHANGE);

     wifiStart();

     configTime(0, 0, NTP0, NTP1);
     setenv("TZ", "EST+5EDT,M3.2.0/2,M11.1.0/2", 3);   // this sets TZ to Indianapolis, Indiana
     tzset();

     Serial.print("wait for first valid timestamp ");

     while (time(nullptr) < 100000ul)
     {
          Serial.print(".");
          delay(10);
     }

     Serial.println(" time synced");

     server.begin();
     Serial.println("Web server running. Waiting for the ESP IP...");

     delay(500);

     // Printing the ESP IP address
     Serial.print("Server IP:  ");
     Serial.println(WiFi.localIP());
     Serial.print("Port:  ");
     Serial.print(LISTEN_PORT);
     Serial.println("\n");

     getDateTime();

     Serial.println(dtStamp);
     Serial.println("");

     ThingSpeak.begin(client);

     SPIFFS.begin();

     SPIFFS.format();

     /*
          SPIFFS.remove("/DAY.TXT");
          SPIFFS.remove("/MONTH.TXT");
          SPIFFS.remove("/YEAR.TXT");
          SPIFFS.remove("/TOTALMO.TXT");
     */

     newDay();

}

/////////
void loop()
{

     wifi_set_sleep_type(NONE_SLEEP_T);
     delay(100);

     if (WiFi.status() != WL_CONNECTED)
     {
          wifiStart();

          delay(1000 * 10);   //wait 10 seconds before writing



          //Open a "WIFI.TXT" for appended writing.   Client access ip address logged.
          File logFile = SPIFFS.open("/WIFI.TXT", "a");

          if (!logFile)
          {
               Serial.println("File failed to open");
          }
          else
          {
               logFile.print("Connected WiFi:  ");
               logFile.println(dtStamp);
          }
     }

     delay(0);

     numberDays();

     getDateTime();

     //if ((HOUR == 23) && (MINUTE == 59) && (SECOND == 58))   //New day; elapsed time keeping processes.
     if ((MINUTE % 5 == 0) && (SECOND == 0))   //newDay function occurs every 5 minutes in testing this version.
     {
          newDay();
          delay(1000);
     }
     else
     {
          listen();
     }

     if (dayFlag == 1)
     {

          totalDay = totalDay + elapsed;
          writeDay();

          Serial.print("");
          Serial.print((totalDay / 60000), 2);
          Serial.println(":  Accumulated Minutes on the Day");
          Serial.println("");
          Serial.println("");

          dayFlag = 0;

     }



}

/////////////
void newDay()
{

     count++;

     totalMonth = totalMonth + totalDay;
     writetotalMonth();
     totalYear = totalMonth;
     
     Serial.println("");
     Serial.println("  --- Yesterday ---");
     Serial.print("");

     readDay();

     Serial.print("  Total Minutes --one Day:  ");
     Serial.print(totalDay / 60000, 2);
     Serial.println("  " + (String)dtStamp);

     readtotalMonth();

     Serial.print("  Total Minutes for ");
     Serial.print(Month);
     Serial.print(":  ");
     Serial.print((totalMonth / 60000), 2);   //Write Minutes
     Serial.println("");

     readYear();

     Serial.print("  Total Minutes Year:  ");
     Serial.print((totalYear / 60000), 2);
     Serial.println("");
     Serial.println("  count:  " + (String)count);
     Serial.println("");

     writeMOYEAR();   //Write totalDay Minutes; daily record.

     //speak();

     totalDay = 0;

     SPIFFS.remove("/DAY.TXT");

     if (count == 4)  //count == days then new month
     {

          totalMonth = 0;

          count = 1;   //initializes for first call to newDay --all values zero

          readYear();
     }

     if ((MONTH == 12) && (DATE == days))
     {
          writeYear();
     }

}

////////////////////////
void writeDay()  //write --total for day in milliseconds.
{

     File logFile = SPIFFS.open("/DAY.TXT", "w");

     if (!logFile)
     {
          Serial.println("  File totalDay failed to open");
          Serial.println("");
     }
     else
     {

          logFile.print(totalDay / 60000);
          logFile.close();

     }
}

///////////////
void readDay()  //read --total for day in milliseconds.
{

     if (SPIFFS.exists("/DAY.TXT"))
     {

          File logFile = SPIFFS.open("/DAY.TXT", "r");

          if (!logFile)
          {
               Serial.println("File totalDay failed to open");
               Serial.println("");
          }
          else
          {
               String tFY = logFile.readStringUntil('\n');
               totalDay = tFY.toInt();

               logFile.close();


          }
     }
}

////////////////////////
void writetotalMonth()  //write --totals for month in milliseconds.
{

     File logFile = SPIFFS.open("/TOTALMO.TXT", "w");

     if (!logFile)
     {
          Serial.println("  File totalMonth failed to open");
          Serial.println("");
     }
     else
     {

          logFile.print(totalMonth);
          logFile.close();

     }
}

///////////////
void readtotalMonth()  //read --totals for month in milliseconds.
{

     File logFile = SPIFFS.open("/TOTALMO.TXT", "r");

     if (!logFile)
     {
          Serial.println("File '/TOTALMO.TXT' failed to open");
          Serial.println("");
     }
     else
     {
          String tFY = logFile.readStringUntil('\n');
          totalMonth = tFY.toInt();

          logFile.close();


     }
}

////////////////
void writeMOYEAR()
{

     String logname;   //Create filename; ("/MO" + MONTH + YEAR)
     logname = "/MO";
     logname += MONTH;
     logname += YEAR;
     logname += ".TXT";


     File logFile = SPIFFS.open(logname.c_str(), "a");

     if (!logFile)
     {
          Serial.println("  File '/MOYEAR.TXT' failed to open");
          Serial.println("");
     }
     else
     {

          String data;   //Create String data = (DATE, totalDay)
          data = DATE;
          data += " , ";
          data += totalDay / 60000;

          logFile.println(data.c_str());
          logFile.close();

     }

}

////////////////////////
void writeYear()  //write --totals for month in milliseconds.
{

     String logname;   //Create filename; ("/YEAR" + YEAR)
     logname = "/YEAR";
     logname += YEAR;
     logname += ".TXT";


     File logFile = SPIFFS.open(logname.c_str(), "w");

     if (!logFile)
     {
          Serial.println("  File writeYear failed to open");
          Serial.println("");
     }
     else
     {

          String year;   //Create String year = Month + " , " + (String)YEAR + " , " + totalMonth/60000
          year = MONTH;
          year += " / ";
          year = YEAR;
          year += " , ";
          year += totalYear;

          logFile.println(year.c_str());
          logFile.close();

     }
}

///////////////
void readYear()  //read --totals for year in milliseconds.
{

     if (SPIFFS.exists("'/YEAR' + YEAR + '.TXT'"))
     {
         String logname;   //Create filename; ("/YEAR" + YEAR)
         logname = "/YEAR";
         logname += YEAR;
         logname += ".TXT";

          File logFile = SPIFFS.open(logname.c_str(), "r");

          if (!logFile)
          {
               Serial.println("File readYear failed to open");
               Serial.println("");
          }
          else
          {
               String tFY = logFile.readStringUntil('\n');
               totalYear = tFY.toInt();

               logFile.close();


          }
     }
}

////////////////////
String getDateTime()
{
     struct tm *ti;

     tnow = time(nullptr) + 1;
     //strftime(strftime_buf, sizeof(strftime_buf), "%c", localtime(&tnow));
     ti = localtime(&tnow);
     DOW = ti->tm_wday;
     YEAR = ti->tm_year + 1900;
     MONTH = ti->tm_mon + 1;
     DATE = ti->tm_mday;
     HOUR  = ti->tm_hour;
     MINUTE  = ti->tm_min;
     SECOND = ti->tm_sec;

     strftime(strftime_buf, sizeof(strftime_buf), "%a , %m/%d/%Y , %H:%M:%S %Z", localtime(&tnow));
     dtStamp = strftime_buf;
     return (dtStamp);

}

///////////////////
String numberDays()
{

     getDateTime();

     switch (MONTH)
     {

          //todo:  Allocate arrary to hold data for every day in a MONTH --for each case (MONTH)

          case 1:
               //January = 31 days
               days = 31;
               Month = "January";
               break;
          case 2:
               //February = 28
               days = 28;
               Month = "February";
               break;
          case 3:
               //March = 31 days
               days = 31;
               Month = "March";
               break;
          case 4:
               //April = 30 days
               days = 30;
               Month = "April";
               break;
          case 5:
               //May = 31 days
               days = 31;
               Month = "May";
               break;
          case 6:
               //June 30 = 31 days
               days = 31;
               Month = "June";
               break;
          case 7:
               //July = 31 days
               days = 31;
               Month = "July";
               break;
          case 8:
               //August = 31days
               days = 6;
               Month = "August";
               break;
          case 9:
               //September = 30
               days = 4;   //30
               Month = "September";
               break;
          case 10:
               //October = 31 days
               days = 31;
               Month = "October";
               break;
          case 11:
               //November = 30 days
               days = 30;
               Month = "November";
               break;
          case 12:
               //December = 31 days
               days = 31;
               Month = "December";
               break;

     }

     return (Month);

}

//////////////
void runTime()
{

     if (dayFlag == 0)
     {
          value = digitalRead (furnace);

          if (value == LOW)
          {
               //Serial.println("Motor ON");
               start = millis();
               dayFlag = 0;
          }

          if (value == HIGH)
          {
               finished = millis();
               elapsed = finished - start;
               dayFlag = 1;
          }
     }

}

////////////
void speak()
{

     if (SPIFFS.exists("/DAY.TXT"))
     {
          readDay();   //Daily Minutes --running total for one Day.
     }

     minutes = totalDay / 60000;

     char t_buffered1[14];
     dtostrf(minutes, 7, 2, t_buffered1);

     // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
     // pieces of information in a channel.  Here, we write to field 1.
     ThingSpeak.setField(1, t_buffered1);  //minutes

     // Write the fields that you've set all at once.
     ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

     getDateTime();

     Serial.println("");
     Serial.println("  Sent data to Thingspeak.com  " + dtStamp + "\n");

     //listen();

}

/////////////
void listen()   // Listen for client connection
{

     if (WiFi.status() != WL_CONNECTED)
     {
          wifiStart();

          delay(1000 * 10);   //wait 10 seconds before writing

          //Open a "WIFI.TXT" for appended writing.   Client access ip address logged.
          File logFile = SPIFFS.open("/WIFI.TXT", "a");

          if (!logFile)
          {
               Serial.println("File failed to open");
          }
          else
          {
               logFile.print("Connected WiFi:  ");
               logFile.println(dtStamp);
          }


     }

     // Check if a client has connected
     client = server.available();

     if (client)
     {

          // Process this request until it completes or times out.
          // Note that this is explicitly limited to handling one request at a time!

          // Clear the incoming data buffer and point to the beginning of it.
          bufindex = 0;
          memset(&buffer, 0, sizeof(buffer));

          // Clear action and path strings.
          memset(&action, 0, sizeof(action));
          memset(&path,   0, sizeof(path));

          // Set a timeout for reading all the incoming data.
          unsigned long endtime = millis() + TIMEOUT_MS;

          // Read all the incoming data until it can be parsed or the timeout expires.
          bool parsed = false;

          while (!parsed && (millis() < endtime) && (bufindex < BUFFER_SIZE))
          {

               if (client.available())
               {

                    buffer[bufindex++] = client.read();

               }

               parsed = parseRequest(buffer, bufindex, action, path);

          }

          if (parsed)
          {

               error = 0;

               // Check the action to see if it was a GET request.
               if (strcmp(action, "GET") == 0)
               {

                    digitalWrite(online, HIGH);  //turn on online LED indicator

                    String ip1String = "10.0.0.146";   //Host ip address
                    String ip2String = client.remoteIP().toString();   //client remote IP address

                    Serial.print("\n");
                    Serial.println("Client connected:  " + dtStamp);
                    Serial.print("Client IP:  ");
                    Serial.println(ip2String);
                    Serial.println(F("Processing request"));
                    Serial.print(F("Action: "));
                    Serial.println(action);
                    Serial.print(F("Path: "));
                    Serial.println(path);

                    //accessLog();

                    // Check the action to see if it was a GET request.
                    if (strncmp(path, "/favicon.ico", 12) == 0)  // Respond with the path that was accessed.
                    {
                         char *filename = "/FAVICON.ICO";
                         strcpy(MyBuffer, filename);

                         // send a standard http response header
                         client.println("HTTP/1.1 200 OK");
                         client.println("Content-Type: image/x-icon");
                         client.println();

                         error = 0;

                         readFile();

                    }
                    // Check the action to see if it was a GET request.
                    if ((strcmp(path, "/Furnace") == 0) || (strcmp(path, "/") == 0))   // Respond with the path that was accessed.
                    {



                         // First send the success response code.
                         client.println("HTTP/1.1 200 OK");
                         client.println("Content-Type: html");
                         client.println("Connnection: close");
                         client.println("Server: Robotdyn WiFi D1 R2");
                         // Send an empty line to signal start of body.
                         client.println("");
                         // Now send the response data.
                         // output dynamic webpage
                         client.println("<!DOCTYPE HTML>");
                         client.println("<html lang='en'>");
                         client.println("<head><title>Furnace Monitor</title></head>");
                         client.println("<body>");
                         // add a meta refresh tag, so the browser pulls again every 15 seconds:
                         //client.println("<meta http-equiv=\"refresh\" content=\"15\">");
                         client.println("<h2>Furnace Monitor<br>");
                         client.println("Lafayette, IN 47904</h2><h3><br>");

                         getDateTime();

                         client.println(dtStamp);
                         client.print("<br><br>");

                         if (lastUpdate != NULL)
                         {
                              client.println("Last Update:  ");
                              client.println(lastUpdate);
                              client.println("<br><br><br>");
                         }

                         //readDay();   //Daily Minutes --running total for one Day.

                         client.print("Total Minutes for the Day:  ");
                         client.println((totalDay / 60000), 2);
                         client.println(" <br><br><br>");

                         //readtotalMonth();

                         client.print("Total Minutes for ");
                         client.print(Month);
                         client.print(":  ");
                         client.println((totalMonth / 60000), 2);
                         client.println(" <br><br><br>");

                         readYear();

                         client.print("  Total Minutes for Year:  ");
                         client.println((totalYear / 60000), 2);
                         client.println(" <br><br>");


                         client.println("<br><br></h3>");

                         links();

                         end();

                    }
                    // Check the action to see if it was a GET request.
                    else if (strcmp(path, "/SdBrowse") == 0)   // Respond with the path that was accessed.
                    {

                         // send a standard http response header
                         client.println("HTTP/1.1 200 OK");
                         client.println("Content-Type: text/html");
                         client.println();
                         client.println("<!DOCTYPE HTML>");
                         client.println("<html>");
                         client.println("<body>");
                         client.println("<head><title>SDBrowse</title><head />");
                         // print all the files, use a helper to keep it clean
                         client.println("<h2>Furnace Monitor Files</h2><h3>");

                         //////////////// Code to listFiles from martinayotte of the "ESP8266 Community Forum" ///////////////
                         String str = String("<html><head></head>");

                         if (!SPIFFS.begin())
                         {
                              Serial.println("SPIFFS failed to mount !");
                         }
                         Dir dir = SPIFFS.openDir("/");
                         while (dir.next())
                         {
                              str += "<a href=\"";
                              str += dir.fileName();
                              str += "\">";
                              str += dir.fileName();
                              str += "</a>";
                              str += "    ";
                              str += dir.fileSize();
                              str += "<br>\r\n";
                         }
                         str += "</body></html>\r\n";

                         client.print(str);
                         client.print("</h3>");

                         ////////////////// End code by martinayotte //////////////////////////////////////////////////////

                         links();

                         end();

                    }
                    // Check the action to see if it was a GET request.
                    else if (strcmp(path, "/Graphs") == 0)   // Respond with the path that was accessed.
                    {

                         //delayTime =1000;

                         // First send the success response code.
                         client.println("HTTP/1.1 200 OK");
                         client.println("Content-Type: html");
                         client.println("Connnection: close");
                         client.println("Server: Robotdyn D1 R2");
                         // Send an empty line to signal start of body.
                         client.println("");
                         // Now send the response data.
                         // output dynamic webpage
                         client.println("<!DOCTYPE HTML>");
                         client.println("<html>");
                         client.println("<body>");
                         client.println("<head><title>Graphed runtime</title></head>");
                         // add a meta refresh tag, so the browser pulls again every 15 seconds:
                         //client.println("<meta http-equiv=\"refresh\" content=\"15\">");
                         client.println("<br>");
                         client.println("<h2>Furnace runtime per day</h2><br>");
                         client.println("<p>");
                         client.println("<frameset rows='30%,70%' cols='33%,34%'>");
                         client.println("<iframe width='450' height='260' style='border: 1px solid #cccccc;' src='https://thingspeak.com/channels/562046/charts/1?bgcolor=%23ffffff&color=%23d62020&dynamic=true&results=60&title=Simulation&type=line&xaxis=Days&yaxis=Minutes'></iframe>");
                         client.println("</frameset>");
                         client.println("<br><br><br><br>");

                         links();

                         end();

                    }
                    else if ((strncmp(path, "/DAY", 4) == 0) ||  (strcmp(path, "/TOTALMO.TXT") == 0) || (strncmp(path, "/MO.TXT", 3) == 0) || (strncmp(path, "/YEAR2018.TXT", 8) == 0))   // Respond with the path that was accessed.
                    {

                         char *filename;
                         char name;
                         strcpy( MyBuffer, path );
                         filename = &MyBuffer[1];

                         if ((strncmp(path, "/FAVICON.ICO", 12) == 0) || (strncmp(path, "/SYSTEM~1", 9) == 0) || (strncmp(path, "/ACCESS", 7) == 0))
                         {

                              client.println("HTTP/1.1 404 Not Found");
                              client.println("Content-Type: text/html");
                              client.println();
                              client.println("<h2>404</h2>");
                              delay(250);
                              client.println("<h2>File Not Found!</h2>");
                              client.println("<br><br>");
                              client.println("<a href=http://" + publicIP + ":" + LISTEN_PORT + "/SdBrowse    >Return to File Browser</a><br>");

                              Serial.println("Http 404 issued");

                              error = 1;

                              end();
                         }
                         else
                         {

                              client.println("HTTP/1.1 200 OK");
                              client.println("Content-Type: text/plain");
                              client.println("Content-Disposition: inline");   //was attachment
                              client.println("Content-Length:");
                              client.println("Connnection: close");
                              client.println();

                              readFile();

                              end();
                         }

                    }
                    // Check the action to see if it was a GET request.
                    else if (strncmp(path, "/Grey111.TXT", 7) == 0)  //replace "/zzzzzzz" with your choice.  Makes "ACCESS.TXT" a restricted file.  Use this for private access to remote IP file.
                    {
                         //Restricted file:  "ACCESS.TXT."  Attempted access from "Server Files:" results in
                         //404 File not Found!

                         char *filename = "/ACCESS.TXT";
                         strcpy(MyBuffer, filename);

                         // send a standard http response header
                         client.println("HTTP/1.1 200 OK");
                         client.println("Content-Type: text/plain");
                         client.println("Content-Disposition: attachment");
                         //client.println("Content-Length:");
                         client.println();

                         readFile();

                         end();

                    }
                    else
                    {

                         delay(1000);

                         // everything else is a 404
                         client.println("HTTP 404 Not Found");
                         client.println("Content-Type: text/html");
                         client.println();
                         client.println("<h2>404</h2>");
                         delay(250);
                         client.println("<h2>File Not Found!</h2>");
                         client.println("<br><br>");
                         client.println("<a href=http://" + publicIP + ":" + LISTEN_PORT + "/SdBrowse    >Return to File Browser</a><br>");

                         Serial.println("Http 404 issued");

                         error = 1;

                         end();

                    }

               }
               else
               {
                    // Unsupported action, respond with an HTTP 405 method not allowed error.
                    Serial.print(action);
                    client.println("HTTP Method Not Allowed");
                    client.println("");
                    Serial.println("");
                    Serial.println("Http 405 issued");
                    Serial.println("");

                    digitalWrite(online, HIGH);   //turn-on online LED indicator

                    error = 2;

                    end();

               }

               accessLog();

          }

     }


}

//********************************************************************
//////////////////////////////////////////////////////////////////////
// Return true if the buffer contains an HTTP request.  Also returns the request
// path and action strings if the request was parsed.  This does not attempt to
// parse any HTTP headers because there really isn't enough memory to process
// them all.
// HTTP request looks like:
//  [method] [path] [version] \r\n
//  Header_key_1: Header_value_1 \r\n
//  ...
//  Header_key_n: Header_value_n \r\n
//  \r\n
bool parseRequest(uint8_t* buf, int bufSize, char* action, char* path)
{
     // Check if the request ends with \r\n to signal end of first line.
     if (bufSize < 2)
          return false;

     if (buf[bufSize - 2] == '\r' && buf[bufSize - 1] == '\n')
     {
          parseFirstLine((char*)buf, action, path);
          return true;
     }
     return false;
}


///////////////////////////////////////////////////////////////////
// Parse the action and path from the first line of an HTTP request.
void parseFirstLine(char* line, char* action, char* path)
{
     // Parse first word up to whitespace as action.
     char* lineaction = strtok(line, " ");

     if (lineaction != NULL)

          strncpy(action, lineaction, MAX_ACTION);
     // Parse second word up to whitespace as path.
     char* linepath = strtok(NULL, " ");

     if (linepath != NULL)

          strncpy(path, linepath, MAX_PATH);
}

////////////
void links()
{

     client.println("<br><br></h3>");
     client.println("<a href=http://" + publicIP + ":" + LISTEN_PORT + "/Furnace >Furnace Monitor</a><br>");
     client.println("<br>");
     client.println("<a href=http://" + publicIP + ":" + LISTEN_PORT + "/SdBrowse >File Browser</a><br>");
     client.println("<br>");
     client.println("<a href=http://" + publicIP + ":" + LISTEN_PORT + "/Graphs >Graphed Runtime</a><br>");
     client.println("<br>");
     //client.println("<a href=http://" + publicIP + ":" + LISTEN_PORT + "/README.TXT download>Server:  README</a><br>");
     client.println("<br>");
     //Show IP Adress on client screen
     client.print("<h3>Client IP: ");
     client.print(client.remoteIP().toString().c_str());
     client.print("</h3>");
     client.println("</body>");
     client.println("</html>");
}

///////////////
void readFile()
{

     digitalWrite(online, HIGH);   //turn-on online LED indicator

     String filename = (const char *)&MyBuffer;

     Serial.print("File:  ");
     Serial.println(filename);

     File webFile = SPIFFS.open(filename, "r");

     if (!webFile)
     {
          Serial.println("File failed to open");
          Serial.println("\n");
     }
     else
     {
          char buf[1024];
          int siz = webFile.size();
          //.setContentLength(str.length() + siz);
          //webserver.send(200, "text/plain", str);
          while (siz > 0)
          {
               size_t len = std::min((int)(sizeof(buf) - 1), siz);
               webFile.read((uint8_t *)buf, len);
               client.write((const char*)buf, len);
               siz -= len;
          }
          webFile.close();
     }

     error = 0;

     delay(1000);

     MyBuffer[0] = '\0';

     digitalWrite(online, LOW);   //turn-off online LED indicator

     listen();

}

/////////////////
void accessLog()
{
     getDateTime();

     String ip1String = "10.0.0.146";   //Host ip address
     String ip2String = client.remoteIP().toString();   //client remote IP address

     //Open a "access.txt" for appended writing.   Client access ip address logged.
     File logFile = SPIFFS.open("/ACCESS.TXT", "a");

     if (!logFile)
     {
          Serial.println("File failed to open");
     }

     if ((ip1String == ip2String) || (ip2String == "0.0.0.0"))
     {

          //Serial.println("IP Address match");
          logFile.close();

     }
     else
     {
          //Serial.println("IP address that do not match ->log client ip address");

          logFile.print("Accessed:  ");
          logFile.print(dtStamp + " -- ");
          logFile.print("Client IP:  ");
          logFile.print(client.remoteIP());
          logFile.print(" -- ");
          logFile.print("Action:  ");
          logFile.print(action);
          logFile.print(" -- ");
          logFile.print("Path:  ");
          logFile.print(path);

          //Serial.println("Error = " + (String)error);


          if ((error == 1) || (error == 2))
          {

               if (error == 1)
               {

                    Serial.println("Error 404");
                    logFile.print("  Error 404");
                    error = 0;

               }

               if (error == 2)
               {

                    Serial.println("Error 405");
                    logFile.print("  Error 405");

               }

          }

          error = 0;

          logFile.println("");
          logFile.close();

     }

}


////////////////
void wifiStart()
{

     WiFi.mode(WIFI_STA);

     //wifi_set_sleep_type(NONE_SLEEP_T);

     Serial.println();
     Serial.print("MAC: ");
     Serial.println(WiFi.macAddress());

     // We start by connecting to a WiFi network
     Serial.print("Connecting to ");
     Serial.println(ssid);


     //setting the addresses
     IPAddress ip(10, 0, 0, 55);
     IPAddress gateway(10, 0, 0, 1);
     IPAddress subnet(255, 255, 255, 0);
     IPAddress dns(75, 75, 75, 75);

     WiFi.begin(ssid, password);

     WiFi.config(ip, gateway, subnet, dns);

     while (WiFi.status() != WL_CONNECTED)
     {
          delay(100);
          Serial.print(".");
     }

     WiFi.waitForConnectResult();

     Serial.printf("Connection result: %d\n", WiFi.waitForConnectResult());

}

//////////
void end()
{

     accessLog();

     // Wait a short period to make sure the response had time to send before
     // the connection is closed .

     delay(1000);

     //Client flush buffers
     client.flush();
     // Close the connection when done.
     client.stop();

     digitalWrite(online, LOW);   //turn-off online LED indicator

     getDateTime();

     Serial.println("Client closed:  " + dtStamp);
     Serial.println("");

     delay(100);   //Delay for changing too quickly to new browser tab.

}





