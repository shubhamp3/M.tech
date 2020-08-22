/*********************************************************************/
/* ********************************************************************
 *  LinkIt One sketch for MQTT Prototype  *                           *
 * Functions:                                                         * 
 * callback        -                                                  *
 * InitLWiFi       -  Choose only one from wifi and gprs              *
 * InitGPRS        -  & make corresponding changes                    *
 * sendMemCard     -                                                  *
 * reconnect       -                                                  *
 * sendAnalogdata  -                                                  *
 * GPS Functions   -                                                  *
 * reportdata      -                                                  *
 * sendreportdata  -                                                  * 
**********************************************************************/
/***********       Linkit One Project- version 2.0     ***************/
/***  Author - Shubham Pandey, Prithesh Kethele, Kanchan Roshan    ***/
/*********************************************************************/

/**************Include Libraries**************/
#include <LGPRS.h>      
#include <LGPRSClient.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <PubSubClient.h>
#include <LSD.h> 
#include <LFlash.h>
#include <LGPS.h>
#include "LDHT.h" 
#include <Wire.h> 
#include <ADXL345.h>  
#include <math.h>
#include <HttpClient.h>
/**************./Include Libraries*****************/


/**************Delay Variables **************/
int reportingDelay = 6000; //All Sensor will be activated for reporting
int sensingDelay = 15000; // Any Sensors can be switched off
int pause = 3000;
/**************./Delay Variables **************/


/**************Local Variables **************/
float locallat, locallong, localtemp;
String slocallat, slocallong, slocaltemp, slocaltilt, slocallight;
double localtilt;
int locallight;
/**************./Local Variables **************/

/**************Global Variables **************/
unsigned long lastSend, lastreportsend;
/**************./Global Variables **************/

/************** Boolean Variables **************/
bool AP_Connect = false;
bool GPRS_Connect = false;
bool server_Connect = false;
bool LGPRS_READY = 1;
/************** Boolean Variables **************/


/**************Server Variables **************/
char tp = '0'; //DHT Switch
char gp = '0'; //GPS Switch
char tS = '0'; //Tilt Switch
char lS = '0'; //Light Switch
float tUT = 70; // Temperature Upper Threshold
float tLT = 20; // Temperature Lower Threshold
double tUP = 195; // Right Tilt Threshold
double uLOW = 165; // Left Tilt Threshold
int mUP = -10;  //Light lower threshold value
int nLOW = 300;  //Light upper threshold value
/**************./Server Variables **************/


/**************Configurations **************/
//---------- DHT Configurations ------------//
#define DHTPIN 8          // what pin we're connected to
#define DHTTYPE DHT22     // using DHT22 sensor
LDHT dht(DHTPIN,DHTTYPE);
float tempC=0.0,Humi=0.0;
//---------- ./DHT Configurations ----------//

//---------- GPS Configurations ----------//
gpsSentenceInfoStruct info; //needed to get GPS data
double latitude = 0.00;
double longitude = 0.00;
double latT = 0.00, lonT = 0.00;
int hour = 0, minute = 0, second = 0;
int sat_num = 0;
int day = 0, month = 0, year = 0;
String time_format = "00:00:00", date_format = "00:00:0000";
String lat_format = "0.00000", lon_format = "0.00000";
//---------- ./GPS Configurations ----------//

//---------- ADXL345 Configurations ----------//
ADXL345 adxl; // adxl is an instance of the ADXL345 library
double xyz[3];
double ax,ay,az; // acc in g (+_ 16g)
double tilt; // range 0 to 360
//---------- ./ADXL345 Configurations ----------//

//---------- Light Sensor Configurations ----------//
#define pinLight A1
int DELAY = 5000;
int lightValue;
//---------- ./Light Sensor Configurations ----------//

//---------- SD Card Configurations ----------//
#define DRV LFlash
//---------- ./SD CardConfigurations ----------//
/**************./Configurations **************/


/**************WiFi Credentials (option 1)**************/
#define WIFI_AP "kanchan"
#define WIFI_PASSWORD "kanchankk"
#define WIFI_AUTH LWIFI_WPA 
LWiFiClient wifiClient;
/**************./WiFi Credentials **************/

/**************GPRS Credentials (option 2)**************/
#define APN "internet" // APN of your cellular provider
#define APN_USER ""
#define APN_PASS ""
LGPRSClient GPRSClient;
HttpClient http(GPRSClient);
/**************./GPRS Credentials **************/

/**************MQTT Credentials **************/
char mqttBroker[] = "ec2-13-59-236-157.us-east-2.compute.amazonaws.com";
//byte mqttBroker[] = {13,59,236,157}; // choose only one
PubSubClient client( wifiClient );
//PubSubClient client( GPRSClient ); // in case of gprs subscribe to this
/**************./MQTT Credentials **************/


/**************Setup function **************/
void setup()
{
    delay(10000);
    Serial.begin(115200);
    Serial.print("HI, LinkIt is now beginning its work\n\n");

    //---------- DHT Setup ----------//
    dht.begin();
    Serial.println(" DHT11 Sensor : ");
    Serial.print( DHTTYPE );
    //---------- ./DHT Setup ----------//

    //---------- GPS Setup ----------//
    LGPS.powerOn();
    Serial.println("\nGPS started.");
    //---------- ./GPS Setup ----------//

    //---------- ADXL345 Setup ----------//
     adxl.powerOn();
    //---------- ./ADXL345 Setup ----------//

    //---------- MQTT Setup ----------//
    client.setServer( mqttBroker, 1883 ); //Setting the mqtt server
    client.setCallback( callback ); //Sets the callback function which will be called when a message is received
    //---------- ./MQTT Setup ----------//

    //---------- Timer Setup ----------//
    lastreportsend = 0;
    lastSend = 0;
    //---------- ./Timer Setup ----------//
}
/**************./Setup function **************/

/**************Forever loop function **************/
void loop()
{
    //----------Initial connect----------//
    if (LWiFi.status() != LWIFI_STATUS_CONNECTED)
    InitLWiFi();

    if (client.state()!=0)
    reconnect();  
    //----------./Initial connect----------//


    //----------Reporting (all sensors enabled)----------//
    if((millis()-lastreportsend) > reportingDelay)
  {
    reportdata();
    if ((localtemp < tLT) || localtemp >tUT || localtilt > tUP || localtilt < uLOW || locallight > mUP || locallight < nLOW)
    {
      Serial.println("Threshold breached Reporting data to server");
      if ( true == ( ( AP_Connect || GPRS_Connect ) && server_Connect ))
      {
          sendreportdata();
          Serial.println("Reporting Data sent to server successfully");
      }
      else
      {
          Serial.println("Processing faild due to connection error\n");
          Serial.println("Retrying...");
          InitLWiFi();
          reconnect();
          sendreportdata();
          Serial.println("Reporting data sent to server successfully");
      }
      
    }
    lastreportsend = millis();
  }
  //----------./Reporting (all sensors enabled)----------//

  //----------Sensing (selected sensors enabled)----------//
  if((millis()- lastSend) >  sensingDelay)
  {
      sendMemCard();
      if ( true == ( ( AP_Connect || GPRS_Connect ) && server_Connect ))
      {
          sendAnalogdata();
          Serial.println("Sensing data sent to server successfully");
      }
      else
      {
          Serial.println("Processing faild due to connection error\n");
          Serial.println("Retrying...");
          InitLWiFi();
          reconnect();
          sendAnalogdata();
          Serial.println("Sensing data sent to server successfully");
      }
      lastSend = millis();
  }

  //----------./Sensing (selected sensors enabled)----------//
  client.loop(); //to maintain connections
}
/**************./Forever loop function **************/

/**************Callback function (when msg recieved) **************/
void callback( char* topic, byte* payload, unsigned int length )
{
      Serial.print( "Recived message on Topic:" );
  Serial.print( topic );
  //Setiing The temperature and gps control values according to the values received from the server
  if (topic[0]=='g')
  {
    if ((char)payload[0]=='0')
      gp = '0';
    else 
      gp ='1'; 
  }
  else
  if (topic[0]=='t')
  {
    if ((char)payload[0]=='0')
      tp = '0';
    else 
      tp ='1'; 
  }
  else
  if (topic[0]=='u')
  {
    tUT=atof((char*)payload);
    Serial.println("Upper Threshold");
    Serial.println(tUT);
  }
  else
  if (topic[0]=='l')
  {
    tLT=atof((char*)payload);
    Serial.println("Lower Threshold");
    Serial.println(tLT);
  }
  else
  if (topic[0]=='r')
  {
    reportingDelay=atoi((char*)payload);
    Serial.println("Reporting Delay: ");
    Serial.println(reportingDelay);
  }
  else
  if (topic[0]=='s')
  {
    sensingDelay=atoi((char*)payload);
    Serial.println("Sensing Delay: ");
    Serial.println(sensingDelay);
  }
  else
  if (topic[0]=='1')
  {
    if ((char)payload[0]=='0')
      tS = '0';
    else 
      tS ='1'; 
  }
  else
  if (topic[0]=='2')
  {
    if ((char)payload[0]=='0')
      lS = '0';
    else 
      lS ='1'; 
  }
  else
  if (topic[0]=='3')
  {
    tUP=atof((char*)payload);
    Serial.println("Tilt Upper Threshold: ");
    Serial.println(tUP);
  }
  else
  if (topic[0]=='4')
  {
    uLOW=atof((char*)payload);
    Serial.println("Tilt Lower Threshold: ");
    Serial.println(uLOW);
  }
  else
  if (topic[0]=='5')
  {
    mUP=atof((char*)payload);
    Serial.println(" Light Upper Threshold: ");
    Serial.println(mUP);
  }
  else
  if (topic[0]=='6')
  {
    nLOW=atof((char*)payload);
    Serial.println("Light Lower Threshold: ");
    Serial.println(nLOW);
  }
  
  Serial.print( "Message:");
  for (int i=0;i<length;i++) 
  {
    Serial.print( (char)payload[i] );
  }
  Serial.println("Switch status of Temperature, GPS Tilt, Light respectively\n");
  Serial.println(tp);
  Serial.println(gp);
  Serial.println(tS);
  Serial.println(lS);
  Serial.println("Switch status changed");

}
/**************./Callback function **************/

/**************sendMemCard function(store data in memory card) **************/
void sendMemCard() 
{
  DRV.begin(); // Read data to send  
  Serial.println("Memory Card Initialized");
  String data_A0; //Temperature Data
  String data_A1; //Latitude Data
  String data_A2; //Longitude Data
  String data_A3; //Tilt Data
  String data_A4; //Light Data
  //----------Temperature sensor----------//
  if (tp=='1')
  {
    //   if(dht.read())
    //   {
        dht.read();
        tempC = dht.readTemperature();
        data_A0 = (String)tempC;
        Serial.println("Temperature data recorded.");
    //   }
    //   else
    //   {
    //     Serial.println("Problem type - severe: DHT read failed.");
    //   }
      
  }
  else
  {
      data_A0 = "f";
      Serial.println("Temperature sensor: switched off, data loaded with value as 'f'");
  }
  //----------./Temperature sensor----------//

  //-----------GPS LAT & LONG---------------//
  if (gp=='1')
  {
    if (getData(&info) > 3)   //Start retriving data if no of satellites being tracked are greater than 3
    {
      String str = "|";
      str += date_format;
      str += ",";
      str += time_format;
      str += ",";
      str += lat_format;
      str += ",";
      str += lon_format;
      str += ",";
      str += sat_num;
      str += "|";
      delay(pause);
    }
    else{
      //Serial.println("Less then 4 satelites found, unable to retrieve location.");
      delay(pause);
    }
     data_A1 = (String)latitude;
     data_A2 = (String)longitude;
     Serial.println("GPS data recorded.");    
  }
  else
  {
     LGPS.powerOff();
     data_A1 = "f";
     data_A2 = "f";
     Serial.println("GPS: switched off, data loaded with value as 'f','f'");
  }
  //-----------./GPS LAT & LONG---------------//

  //-----------Tilt Sensor---------------//
 if(tS == '1')
 {
     adxl.getAcceleration(xyz);
     ax = xyz[0];
     ay = xyz[1];
     az = xyz[2];
     tilt = ( ( (atan2(ay,az) * 180) / 3.14 ) + 180 );// 0 to 360 degree sensing
     data_A3 = (String)tilt;
     Serial.println("Tilt data recorded.");
 }
 else
 {
     data_A3 = "f";
     Serial.println("Temperature sensor: switched off, data loaded with value as 'f'");
 } 
  //-----------./Tilt Sensor---------------//

  //-----------Light Sensor---------------//
 if(lS == '1')
 {
     lightValue = analogRead(pinLight);
     data_A4 = (String)lightValue;
     Serial.println("Light data recorded.");
 }
 else
 {
     data_A4 = "f";
 }
  //-----------Light Sensor---------------//
   // Just debug messages
  Serial.print( "Data sent to sd card, verify it with stored data  : [" );
  Serial.print( data_A0 );
  Serial.print( data_A1 ); 
  Serial.print( data_A2 );
  Serial.print( data_A3 );
  Serial.print( data_A4 );
  Serial.print( "]   -> " );

  // Prepare a JSON payload string
  String payload = "{";
  payload += "\"Temperature\":\""; payload += data_A0; payload += "\", ";
  payload += "\"Latitude\":\""; payload += data_A1; payload += "\", ";
  payload += "\"Longitude\":\""; payload += data_A2; payload += "\", ";
  payload += "\"Tilt\":\""; payload += data_A3; payload += "\", ";
  payload += "\"Light\":\""; payload += data_A4; payload += "\"";  
  payload += "}";

  //Store Payload
  LFile dataFile = DRV.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) 
  {
    dataFile.print(payload);
    dataFile.close();
    Serial.println(payload);
    Serial.println("All data sucessfully stored in SD card");
  }
}  
/**************./sendMemCard function(store data in memory card) **************/


/**********sendAnalogdata function (send memory card data to server)***********/
void sendAnalogdata()
{
  LFile dataFile = DRV.open("datalog.txt");
  // if the file is available, read from it:
  
  if (dataFile) 
  {
    while(dataFile.available())
    {
      Serial.println("Data transfer in progress... : SD card to server");
      int i=1;
      String payload;
      char buf;
      buf = char(dataFile.read());
      payload = String(buf);
      while (buf!='}')
      {
        buf = char(dataFile.read());
        payload += String(buf);
        i=i+1; 
      }
      // Send payload
      char analogData[100];
      Serial.println(payload);
      payload.toCharArray( analogData, 100 ); //copies the contents of the payload to the buffer 'analogData'
      client.publish( "sidsat", analogData ); //publishes to the topic sidsat
      Serial.println( analogData );
      Serial.println("SD card data successfully sent to server.");
      delay(500);
    }
    dataFile.close();
    DRV.remove((char *)"datalog.txt");
    dataFile.close();

  }
}
/**********sendAnalogdata function (send memory card data to server)***********/

/****** report data function(to check threshold value after reporting time) **********/
void reportdata() 
{
  Serial.println("Analysing data for threshold: \n\n");
  //----------Temperature sensor----------//
//   if(dht.read())
//   {  
        dht.read(); 
        Serial.println("Starting to read temperature...\n");
        tempC = dht.readTemperature();
        localtemp = tempC;
//   }
    delay(2000);
  //----------./Temperature sensor----------//

  //----------GPS Data LAT & LONG----------//
  if (getData(&info) > 3)   //Start retriving data if no of satellites being tracked are greater than 3
    {
      String str = "|";
      str += date_format;
      str += ",";
      str += time_format;
      str += ",";
      str += lat_format;
      str += ",";
      str += lon_format;
      str += ",";
      str += sat_num;
      str += "|";
    }
    else
    {
     //Serial.println("Less then 4 satelites.");
     delay(pause);
    }
     locallat = latitude;
     locallong = longitude;
     Serial.println("GPS data recorded."); 
  //----------./GPS Data LAT & LONG----------//

  //---------------Tilt sensor--------------//

      adxl.getAcceleration(xyz);      
      ax = xyz[0];
      ay = xyz[1];
      az = xyz[2];
      Serial.println("Starting to read tilt value...\n");      
      tilt = ( ( (atan2(ay,az) * 180) / 3.14 ) + 180 );
      localtilt = tilt;

  //--------------./Tilt sensor--------------//

  //-------------Light sensor---------------//
      Serial.println("Starting to read Light value...\n"); 
      lightValue = analogRead(pinLight);
      locallight = lightValue;
  //-------------./Light sensor--------------//
  Serial.println("Data is being compared with threshold...exit if no threshold breach found"); 
}
/******./reportdata function(to check threshold value after reporting time) **********/


/******sendreportdata function(Report data to server if threshold breached) **********/
void sendreportdata() 
{
    //--------------Temperature sensor--------------//
//   if(dht.read())
//   {  
        dht.read(); 
        tempC = dht.readTemperature();
        slocaltemp = (String)tempC;
        Serial.println("Collecting temperature value...");
//   }
    delay(2000);
    //--------------./Temperature sensor--------------//

    //--------------GPS Data LAT & LONG--------------//
    if (getData(&info) > 3)   //Start retriving data if no of satellites being tracked are greater than 3
    {
      String str = "|";
      str += date_format;
      str += ",";
      str += time_format;
      str += ",";
      str += lat_format;
      str += ",";
      str += lon_format;
      str += ",";
      str += sat_num;
      str += "|"; 
    }
    else
    {
        //Serial.println("Less then 4 satelites.");
        delay(pause);
    }
     slocallat = (String)latitude;
     slocallong = (String)longitude;
    
    //--------------./GPS Data LAT & LONG--------------//

    //-------------------Tilt sensor-----------------//
      adxl.getAcceleration(xyz);      
      ax = xyz[0];
      ay = xyz[1];
      az = xyz[2];
      Serial.println("Collecting tilt value...");      
      tilt = ( ( (atan2(ay,az) * 180) / 3.14 ) + 180 );
      slocaltilt = (String)tilt;
    //-----------------./Tilt sensor----------------//

    //-----------------Light sensor-----------------//
    lightValue = analogRead(pinLight);
    Serial.println("Collecting Light value...");
    slocallight = (String)lightValue;

    //-----------------./Light sensor------==--------//

  String payload = "{";
  payload += "\"Temperature\":\""; payload += slocaltemp; payload += "\", ";
  payload += "\"Latitude\":\""; payload += slocallat; payload += "\", ";
  payload += "\"Longitude\":\""; payload += slocallong; payload += "\", ";
  payload += "\"Tilt\":\""; payload += slocaltilt; payload += "\", ";
  payload += "\"Light\":\""; payload += slocallight; payload += "\"";  
  payload += "}";

      char analogData[100];
      Serial.println(payload);
      payload.toCharArray( analogData, 100 ); //copies the contents of the payload to the buffer 'analogData'
      client.publish( "sidsat", analogData ); //publishes to the topic sidsat
      Serial.println( analogData );
      Serial.println("Threshold breach reported to server");
      delay(500);
}
/******./sendreportdata function(Report data to server if threshold breached) **********/


/*****************************GPS function **************************/
/**
*Converts degrees from (d)ddmm.mmmm to (d)dd.mmmmmm
*@param str the string rappresentation of the angle in (d)ddmm.mmmm format
*@param dir if true the direction is south, and the angle is negative.
*@return the given angle in dd.mmmmmm format.
*/
float convert(String str, boolean dir)
{
  double mm, dd;
  int point = str.indexOf('.');
  dd = str.substring(0, (point - 2)).toFloat();
  mm = str.substring(point - 2).toFloat() / 60.00;
  return (dir ? -1 : 1) * (dd + mm);
}

/**
*Gets gps informations
*@param info gpsSentenceInfoStruct is a struct containing NMEA sentence infomation
*@return the number of hooked satellites, or 0 if there was an error getting informations
*/
int getData(gpsSentenceInfoStruct* info)
{
  Serial.println("Collecting GPS data.");
  LGPS.getData(info);
//  Serial.println((char*)info->GPGGA);
  if (info->GPGGA[0] == '$')
  {
    Serial.println("Parsing GGA data....");
    String str = (char*)(info->GPGGA);
    str = str.substring(str.indexOf(',') + 1);
    hour = str.substring(0, 2).toInt();
    minute = str.substring(2, 4).toInt();
    second = str.substring(4, 6).toInt();
    time_format = "";
    time_format += hour;
    time_format += ":";
    time_format += minute;
    time_format += ":";
    time_format += second;
    str = str.substring(str.indexOf(',') + 1);
    latitude = convert(str.substring(0, str.indexOf(',')), str.charAt(str.indexOf(',') + 1) == 'S');
    int val = latitude * 1000000;
    String s = String(val);
    lat_format = s.substring(0, (abs(latitude) < 100) ? 2 : 3);
    lat_format += '.';
    lat_format += s.substring((abs(latitude) < 100) ? 2 : 3);
    str = str.substring(str.indexOf(',') + 3);
    longitude = convert(str.substring(0, str.indexOf(',')), str.charAt(str.indexOf(',') + 1) == 'W');
    val = longitude * 1000000;
    s = String(val);
    lon_format = s.substring(0, (abs(longitude) < 100) ? 2 : 3);
    lon_format += '.';
    lon_format += s.substring((abs(longitude) < 100) ? 2 : 3);

    sat_num = str.substring(0, 2).toInt();

    String Latitude = String(latitude, 5);
    String Longitude = String(longitude, 5);
    String Sat_num = String(sat_num);
    
  }
}
/**************./GPS function **************/

/**************Intializing WiFi **************/
void InitLWiFi()
{
  LWiFi.begin();

  // Keep retrying until connected to AP
  Serial.println("Connecting to AP");
  int i=1, j=0, k=3; 
  while (i <= k) 
  {
    LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD));
    if (LWiFi.status() == LWIFI_STATUS_CONNECTED)
    {
      j=1;
      Serial.println("Connected");
      AP_Connect = true;
      break; 
    }
    delay(1000); //Reconnect after 1 second
    i=i+1;
    AP_Connect = false;
    Serial.println("Disconnected");
  }
  if (j==1)
  {Serial.println("Connected to AP");}
  else
  {Serial.println("Could not Connect To AP");
  InitGPRS();
  }
}
/**************./Intializing WiFi **************/

/**************Intializing GPRS **************/
void InitGPRS()
{
  // Keep retrying until connected to AP
  Serial.println("Connecting to GPRS");
  int i=1, j=0, k=3; 
  while (i <= k) 
  {
    LGPRS.attachGPRS(APN,APN_USER,APN_PASS);
    if (LGPRS.attachGPRS(APN,APN_USER,APN_PASS) != LGPRS_READY)
    {
      j=1;
      Serial.println("Connected");
      GPRS_Connect = true;
      break; 
    }
    delay(1000); //Reconnect after 1 second
    i=i+1;
    GPRS_Connect = false;
    Serial.println("Disconnected");
  }
  if (j==1)
  {Serial.println("Connected to GPRS");}
  else
  {Serial.println("Network Failure\n");
  Serial.println("Retrying...");

  InitLWiFi();  
  }   
}
/**************./Intializing GPRS **************/

/**************Intializing MQTT **************/
void reconnect() 
{
  // Loop until we're reconnected
  int i=1;
  while (i<=1) 
  {
    Serial.print("Connecting to MQTT broker ...");

    // Attempt to connect
    if ( client.connect("LinkIt One Client") ) // Connects to the server with the Client ID "Linkit One Client"
    {  
      Serial.println( "[DONE]" );

      // Subscribe to topic "inTopic"
      client.subscribe( "temp" );
      client.subscribe( "gps" );
      client.subscribe( "up" );
      client.subscribe( "low" );
      client.subscribe( "report" );
      client.subscribe( "sense" );
      client.subscribe( "1tilt" );
      client.subscribe( "2light" );
      client.subscribe( "4ulow" );
      client.subscribe( "3tup" );
      client.subscribe( "5mup" );
      client.subscribe( "6nlow" );

      server_Connect = true;
      break;
    } 
    else 
    {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() ); //Gives the reason for not being able to connect
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
      server_Connect = false;
      i=i+1;
    }
  }
}
/**************./Intializing MQTT **************/