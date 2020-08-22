/* 
 *  LinkIt One sketch for MQTT Prototype  *
Functions:
  callback        -
  sendMemCard     -
  InitLWiFi       - 
  reconnect       -
  sendAnalogdata  -
  GPS Functions   - 
  reportdata      -
  sendreportdata  -
*/
//---------------Linkit One IOT Project-----------------//
#include <LGPRS.h>      //include the base GPRS library
#include <LGPRSClient.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <PubSubClient.h>
#include <LSD.h> 
#include <LFlash.h>
#include <LGPS.h>
#include "LDHT.h" // LDHT Library
#include <Wire.h> // For ADXL345 Sensor
#include <ADXL345.h>  //ADXL345 Library
#include <math.h>
#include <HttpClient.h>
#include <math.h>
#define R0 1022
//<<<<<----- Delay Variables ----->>>>>//
int reportingDelay = 5000;
int sensingDelay = 60000;
int pause = 3000;   //time delay in milliseconds for every sensor
//<<<<<----- ./Delay Variables ----->>>>>//

float locallat, locallong, localtemp;
String slocallat, slocallong, slocaltemp, slocaltilt, slocallight,slocalgas;
double localtilt,localgas;
double llatitude;
double llongitude;
int locallight;
double lattarget=33.4097;//this is the latitute to target
double longtarget=82.9643;//this is the longdidute to target
double latrange=0.0001;//this is the offset for how exact it needs to be
double longrange=0.0001;//this is the offset for how exact it needs to be

//---------- DHT Configurations ----------//
#define DHTPIN 8          // what pin we're connected to
//#define DHTTYPE DHT11     // using DHT11 sensor
#define DHTTYPE DHT22     // using DHT22 sensor

LDHT dht(DHTPIN,DHTTYPE);

float tempC=0.0,Humi=0.0;
//---------- ./DHT Configurations ----------//


//---------- GPS Configurations ----------//
gpsSentenceInfoStruct info; //needed to get GPS data

//double latitude = 0.0000;
//double longitude = 0.00;
double latT = 0.00, lonT = 0.00;
int hour = 0, minute = 0, second = 0;
int sat_num = 0;    //number of visible satellites
int day = 0, month = 0, year = 0;
String time_format = "00:00:00", date_format = "00:00:0000";
String lat_format = "0.00000", lon_format = "0.00000";
//---------- ./GPS Configurations ----------//

//---------- ADXL345 Configurations ----------//
ADXL345 adxl; //variable adxl is an instance of the ADXL345 library
double xyz[3];
double ax,ay,az;
double tilt;
//---------- ./ADXL345 Configurations ----------//

//---------- Light Sensor Configurations ----------//
#define pinLight A1
int DELAY = 5000;
int lightValue;
//---------- ./Light Sensor Configurations ----------//

//<<<<<----- Server Variables ----->>>>>//
char tp = '0'; //DHT Switch
char gp = '0'; //GPS Switch
char tS = '0'; //Tilt Switch
char lS = '0'; //Light Switch
float tUT = 70; // Temperature Upper Threshold
float tLT = 0; // Temperature Lower Threshold
double tUP = 210; // Right Tilt Threshold
double uLOW = 150; // Left Tilt Threshold
int mUP = 300;  //Light lower threshold value
int nLOW = -10;  //Light upper threshold value
//<<<<<----- ./Server Variables ----->>>>>//

/* 
  Modify to your WIFI Access Credentials. 
*/

#define APN "internet" // APN of your cellular provider
#define APN_USER ""
#define APN_PASS ""
#define WIFI_AP "kanchank"
#define WIFI_PASSWORD "kanchankk"
#define WIFI_AUTH LWIFI_WPA  // choose from LWIFI_OPEN, LWIFI_WPA, or LWIFI_WEP.
#define DRV LFlash
/*
  Modify to your MQTT b66666666666666666644444444446666666666666666444444roker - Select only one
*/
char mqttBroker[] = "ec2-13-59-236-157.us-east-2.compute.amazonaws.com";
///byte mqttBroker[] = {13,59,236,157}; // modify to your local broker
          //char tp = '0'; //Temperature Control 
          //char gp = '0'; //GPS Control
//LWiFiClient wifiClient;
LGPRSClient wifiClient;
HttpClient http(wifiClient);
PubSubClient client( wifiClient );
int lastSend, lastreportsend;

bool AP_Connect = false;
bool server_Connect = false;

//--------------------------------------------------------------------------
void setup()
{
   delay(10000);  //10s fixed delay
   
   Serial.begin(115200);
   Serial.print("HI, LinkIt is now beginning its work\n\n\n\n");
   
  //---------- DHT Setup ----------//
   dht.begin();
   Serial.println(" DHT11 Sensor :\n");
   Serial.print(DHTTYPE);
  //---------- ./DHT Setup ----------//

  //---------- GPS Setup ----------//
    LGPS.powerOn();
    Serial.println("GPS started.");
  //---------- ./GPS Setup ----------//

  //---------- ADXL345 Setup ----------//
   adxl.powerOn();
   
  //---------- ./HCHO Setup ----------//
//  Serial.print("GAS Sensor started.");
   
   client.setServer( mqttBroker, 1883 ); //Setting the mqtt server
   client.setCallback( callback ); //Sets the callback function which will be called when a message is received

   lastreportsend = 0;
   lastSend = 0; //Denotes the time elapsed from the start of the program to the point when the data was sent via publish command    
}

//--------------------------------------------------------------------------
void loop()
{
  
//----------Initial connect----------//
//Serial.println("Attaching to GPRS network");   // Attach to GPRS network - need to add timeout
if (!LGPRS.attachGPRS(APN,APN_USER,APN_PASS)) 
{
 AP_Connect = true;
  //InitLWiFi();
delay(500);
}
  AP_Connect = true;
Serial.println("GPRS connect!");


// if (LWiFi.status() != LWIFI_STATUS_CONNECTED)
//   InitLWiFi();

 if (client.state()!=0)
   reconnect();  
//----------./Initial connect----------//  
  if((millis()-lastreportsend) > reportingDelay)
  {
    reportdata();
    if ((localtemp < tLT) || localtemp >tUT || localtilt > tUP || localtilt < uLOW || locallight > mUP || locallight < nLOW)
    {
      while (!LGPRS.attachGPRS(APN,APN_USER,APN_PASS)) 
{
  //InitLWiFi();
delay(500);
}
//      if (LWiFi.status() != LWIFI_STATUS_CONNECTED)
//      InitLWiFi();

      if (client.state()!=0)
      reconnect();

      if ( AP_Connect && server_Connect)
      sendreportdata();
    }
    lastreportsend = millis();
  }
  
  if((millis()- lastSend) >  sensingDelay)
  {
    while (!LGPRS.attachGPRS(APN,APN_USER,APN_PASS)) 
{
  //InitLWiFi();
delay(500);
}
//    if (LWiFi.status() != LWIFI_STATUS_CONNECTED)
//    InitLWiFi();  //Function to connect to the Wifi
    Serial.println(AP_Connect);
    Serial.println(server_Connect);
  
    if (client.state()!=0)
    reconnect();
    //if( !client.connected() ) 
   
    delay(5000);
    //if( millis()-lastSend > 5000 ) // millis() returns the time elapsed from the start of the program. Subtracting millis and lastsend gives the time elapsed between sending successive messages
                                 // Here we are Sending an update only after 5 seconds
    { 
      
      sendreportdata();
      //sendAnalogData();
      sendMemCard();
      if ( AP_Connect && server_Connect)
      sendAnalogdata();
    //lastSend = millis();
      lastreportsend = millis();
   
    }
  }
  client.loop();
    
}

//--------------------------------------------------------------------------
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
    Serial.println("Reporting Delay");
    Serial.println(reportingDelay);
  }
  else
  if (topic[0]=='s')
  {
    sensingDelay=atoi((char*)payload);
    Serial.println("Sensing Delay");
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
    Serial.println("Tilt Upper Threshold");
    Serial.println(tUP);
  }
  else
  if (topic[0]=='4')
  {
    uLOW=atof((char*)payload);
    Serial.println("Tilt Lower Threshold");
    Serial.println(uLOW);
  }
  else
  if (topic[0]=='5')
  {
    mUP=atof((char*)payload);
    Serial.println(" Light Upper Threshold");
    Serial.println(mUP);
  }
  else
  if (topic[0]=='6')
  {
    nLOW=atof((char*)payload);
    Serial.println("Light Lower Threshold");
    Serial.println(nLOW);
  }
  
  Serial.print( "Message:");
  for (int i=0;i<length;i++) 
  {
    Serial.print( (char)payload[i] );
  }

  //Printing the final values
  Serial.println();
  Serial.println(tp);
  Serial.println(gp);
  Serial.println(tS);
  Serial.println(lS);
  Serial.println();
}

//--------------------------------------------------------------------------
void sendMemCard() 
{
   DRV.begin();
  // Read data to send
  Serial.println("Memory Card Entered");
  String data_A0; //Temperature Data
  String data_A1; //Latitude Data
  String data_A2; //Longitude Data
  String data_A3; //Tilt Data
  String data_A4; //Light Data
  String data_A5; //Gas Data
  
  if (tp=='1')
  {
   //---------- DHT Loop ----------//
      if(dht.read())
      {   
        Serial.println("Statring to read temperature");
        tempC = dht.readTemperature();
        //Humi = dht.readHumidity();

//##### For testing purposes of DHT #####//
/*        Serial.println("---------------");
        Serial.print("Temperature Celsius = ");
        Serial.print(dht.readTemperature());
        Serial.println("C");
*/
//##### Readings to be added later #####//
/* 
        Serial.print("Temperature Fahrenheit = ");
        Serial.print(dht.readTemperature(false));
        Serial.println("F");

        Serial.print("Humidity = ");
        Serial.print(dht.readHumidity());
        Serial.println("%");

        Serial.print("HeatIndex = ");
        Serial.print(dht.readHeatIndex(tempC,Humi));
        Serial.println("C");

        Serial.print("DewPoint = ");
        Serial.print(dht.readDewPoint(tempC,Humi));
        Serial.println("C");
*/   }
    delay(2000);
  //---------- ./DHT Loop ----------//


    data_A0 = (String)tempC;
    //data_A0 = "0.00";
  }
  else
   data_A0 = "f";

   if (gp=='1') 
   {
     

  //---------- GPS Loop ----------//
//  Serial.println("----------------*");
//    if (getData(&info) > 3)   //Start retriving data if no of satellites being tracked are greater than 3
//    {
//      String str = "|";
//      str += date_format;
//      str += ",";
//      str += time_format;
//      str += ",";
//      str += lat_format;
//      str += ",";
//      str += lon_format;
//      str += ",";
//      str += sat_num;
//      str += "|";
//  //    Serial.println(str);
//   
//    }
//    else{
//      Serial.println("Less then 4 satelites.");
//       
//    }
//    delay(pause);
  //---------- ./GPS Loop ----------//

    data_A1 = (String)llatitude;
      data_A2 = (String)llongitude;

   }
   else
   {
     LGPS.powerOff();
     data_A1 = "f";
    data_A2 = "f";
   }

//******************************

if(tS == '1'){
  //---------- /ADXL345 Loop ----------//
      adxl.getAcceleration(xyz);
      
      ax = xyz[0];
      ay = xyz[1];
      az = xyz[2];
      
      tilt = ( ( (atan2(ay,az) * 180) / 3.14 )+180);
//      tilt = tilt - 180; //-To check;
    
    //########## For Testing Purposes ##########//
//      if(tilt >= 195){
//        rightTiltAlert();
//        return;
//      }
//      else if(tilt <= 165){
//        leftTiltAlert();
//        return;
//      }
//      else{
//        printValues();    
//      }  
    //########## ./For Testing Purposes ##########//
  //---------- ./ADXL345 Loop ----------//

    data_A3 = (String)tilt;
}
else{
  data_A3 = "f";
}

if(lS == '1'){
  //---------- Light Sensor Loop --------//
      lightValue = analogRead(pinLight);

    //########## For Testing Purposes ##########//
//      if(lightValue < lightThresh){
//        Serial.print("\n\nAlert!!\nLight lesser than the Threshold value by ");
//        Serial.print(lightThresh - lightValue);
//        delay(1000);
//        return;
//      }
//    
//      Serial.print("\nValue of Light  : ");
//      Serial.print(lightValue);
//      Serial.println();
    //########## ./For Testing Purposes ##########//
  //---------- ./Light Sensor Loop --------//  

      data_A4 = (String)lightValue;
}
else{
  data_A4 = "f";
}
//---------- Gas Sensor Loop --------//
if(lS == '1'){
int sensorValue=analogRead(A0);
float Resistance=(1023.0/sensorValue)-1;
delay(500);
double ppm= pow(10.0,((log10(Resistance/R0)-0.0827)/(-0.4807)));
data_A5 = (String)ppm;
}
else{
  data_A5 = "f";
}
//******************************

  // Just debug messages
  Serial.print( "Sending analog data : [" );
  Serial.print( data_A0 ); 
  Serial.print( data_A1 ); 
  Serial.print( data_A2 );
  Serial.print( data_A3 );
  Serial.print( data_A4 );
  Serial.print( data_A5 );
  Serial.print( "]   -> " );

  // Prepare a JSON payload string
  String payload = "{";
  payload += "\"Temperature\":\""; payload += data_A0; payload += "\", ";
  payload += "\"Latitude\":\""; payload += data_A1; payload += "\", ";
  payload += "\"Longitude\":\""; payload += data_A2; payload += "\", ";
  payload += "\"Tilt\":\""; payload += data_A3; payload += "\", ";
  payload += "\"Light\":\""; payload += data_A4; payload += "\",  ";  
  payload += "\"GAS Sensor Value\":\""; payload += data_A5; payload += "\""; 
  payload += "}";

  //Store Payload
  LFile dataFile = DRV.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) 
  {
    Serial.println("Memory Data Entered");
    dataFile.print(payload);
    dataFile.close();
    Serial.println(payload);
  }
  
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}


//--------------------------------------------------------------------------
//void InitLWiFi()
//{
//
//  Serial.println("Attaching to GPRS network");   // Attach to GPRS network - need to add timeout
//while (!LGPRS.attachGPRS(APN,APN_USER,APN_PASS)) 
//{
//delay(500);
//}
//  
//Serial.println("GPRS attached!");
//delay(10000);
//  LWiFi.begin();
//
//  // Keep retrying until connected to AP
//  Serial.println("Connecting to AP");
//  int i=1, j=0;
//  while (i <= 1) 
//  {
//    LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD));
//    if (LWiFi.status() == LWIFI_STATUS_CONNECTED)
//    {
//      j=1;
//      Serial.println("Connected");
//      AP_Connect = true;
//      break; 
//    }
//    delay(10000); //Reconnect after 1 second
//    i=i+1;
//    AP_Connect = false;
//    Serial.println("Disconnected");
//  }
//  if (j==1)
//  Serial.println("Connected to AP");
//  else
//  Serial.println("Could not Connect To AP");

  
//}


//--------------------------------------------------------------------------
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

      // Publish a message on topic "outTopic"
      client.publish( "sidsat","Hello, This is LinkIt One" );
      // Subscribe to topic "inTopic"
      client.subscribe( "temp" );
      client.subscribe( "gps" );
      client.subscribe( "up" );
      client.subscribe( "low" );
      client.subscribe( "report" );
      client.subscribe( "sense" );
      //client.subscribe( "hum" );
      client.subscribe( "1tilt" );
      client.subscribe( "2light" );
      client.subscribe( "4ulow" );
      client.subscribe( "3tup" );
      //client.subscribe( "ilow" );
      //client.subscribe( "hup" );
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
    //!client.connected() can also be used
  }
}

//--------------------------------------------------------------------------
void sendAnalogdata()
{
  LFile dataFile = DRV.open("datalog.txt");
  // if the file is available, read from it:
  
  if (dataFile) 
  {
    while(dataFile.available())
    {
      Serial.println("Send Entered");
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
      delay(500);
    }
    dataFile.close();
    DRV.remove("datalog.txt");
    dataFile.close();
 
  }
}



 //'''''''''' GPS Functions ''''''''''//
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
//int getData(gpsSentenceInfoStruct* info)
//{
//  Serial.println("Collecting GPS data.");
//  LGPS.getData(info);
////  Serial.println((char*)info->GPGGA);
//  if (info->GPGGA[0] == '$')
//  {
//    Serial.println("Parsing GGA data....");
//    String str = (char*)(info->GPGGA);
//    str = str.substring(str.indexOf(',') + 1);
//    hour = str.substring(0, 2).toInt();
//    minute = str.substring(2, 4).toInt();
//    second = str.substring(4, 6).toInt();
//    time_format = "";
//    time_format += hour;
//    time_format += ":";
//    time_format += minute;
//    time_format += ":";
//    time_format += second;
//    str = str.substring(str.indexOf(',') + 1);
//    latitude = convert(str.substring(0, str.indexOf(',')), str.charAt(str.indexOf(',') + 1) == 'S');
//    int val = latitude * 1000000;
//    String s = String(val);
//    lat_format = s.substring(0, (abs(latitude) < 100) ? 2 : 3);
//    lat_format += '.';
//    lat_format += s.substring((abs(latitude) < 100) ? 2 : 3);
//    str = str.substring(str.indexOf(',') + 3);
//    longitude = convert(str.substring(0, str.indexOf(',')), str.charAt(str.indexOf(',') + 1) == 'W');
//    val = longitude * 1000000;
//    s = String(val);
//    lon_format = s.substring(0, (abs(longitude) < 100) ? 2 : 3);
//    lon_format += '.';
//    lon_format += s.substring((abs(longitude) < 100) ? 2 : 3);
//
//    sat_num = str.substring(0, 2).toInt();
//
////    String Latitude = String(latitude, 5);
////    String Longitude = String(longitude, 5);
//  String Sat_num = String(sat_num);
//    
//  }
//}
//'''''''''' ./GPS Functions ''''''''''//





//--------------------------------------------------------------------------
void reportdata() 
{
  GPS_receive();
  Serial.println("Sensing data for threshold \n\n");
  if(dht.read())
  {   
        Serial.println("Starting to read temperature");
        tempC = dht.readTemperature();
        localtemp = tempC;
        //Humi = dht.readHumidity();

//##### For testing purposes of DHT #####//
/*      Serial.println("---------------");
        Serial.print("Temperature Celsius = ");
        Serial.print(dht.readTemperature());
        Serial.println("C");
*/
//##### Readings to be added later #####//
/* 
        Serial.print("Temperature Fahrenheit = ");
        Serial.print(dht.readTemperature(false));
        Serial.println("F");

        Serial.print("Humidity = ");
        Serial.print(dht.readHumidity());
        Serial.println("%");

        Serial.print("HeatIndex = ");
        Serial.print(dht.readHeatIndex(tempC,Humi));
        Serial.println("C");

        Serial.print("DewPoint = ");
        Serial.print(dht.readDewPoint(tempC,Humi));
        Serial.println("C");
*/   }
    delay(2000);
  //---------- ./DHT Loop ----------//

  //---------- GPS Loop ----------//
//  Serial.println("----------------*");
//    if (getData(&info) > 3)   //Start retriving data if no of satellites being tracked are greater than 3
//    {
//      String str = "|";
//      str += date_format;
//      str += ",";
//      str += time_format;
//      str += ",";
//      str += lat_format;
//      str += ",";
//      str += lon_format;
//      str += ",";
//      str += sat_num;
//      str += "|";
//  //    Serial.println(str);
//    }
//    else
//    Serial.println("Less then 4 satelites.");
//    delay(pause);
//  //---------- ./GPS Loop ----------//
    
    locallat = llatitude;
    locallong = llongitude;

//******************************
  //---------- ADXL345 Loop ----------//
      adxl.getAcceleration(xyz);
      
      ax = xyz[0];
      ay = xyz[1];
      az = xyz[2];
      
      tilt = ( ( (atan2(ay,az) * 180) / 3.14 ) + 180 );
    
    //########## For Testing Purposes ##########//
//      if(tilt >= 195){
//        rightTiltAlert();
//        return;
//      }
//      else if(tilt <= 165){
//        leftTiltAlert();
//        return;
//      }
//      else{
//        printValues();    
//      }  
    //########## ./For Testing Purposes ##########//
  //---------- ./ADXL345 Loop ----------//

  localtilt = tilt;
  
  //---------- Light Sensor Loop --------//
      lightValue = analogRead(pinLight);
    
    //########## For Testing Purposes ##########//
//      if(lightValue < lightThresh){
//        Serial.print("\n\nAlert!!\nLight lesser than the Threshold value by ");
//        Serial.print(lightThresh - lightValue);
//        delay(1000);
//        return;
//      }
//  
//      Serial.print("\nValue of Light  : ");
//      Serial.print(lightValue);
//      Serial.println();
    //########## ./For Testing Purposes ##########//
  //---------- ./Light Sensor Loop --------//
    locallight = lightValue;
//******************************
//---------- Gas Sensor Loop --------//
int sensorValue=analogRead(A0);
float Resistance=(1023.0/sensorValue)-1;
delay(500);
double ppm= pow(10.0,((log10(Resistance/R0)-0.0827)/(-0.4807)));
localgas=ppm;
}
//--------------------------------------------------------------------------
void sendreportdata() 
{
  GPS_receive();
  Serial.println( "Sensing data for threshold \n\n");
  if(dht.read())
  {   
        Serial.println("Starting to read temperature");
        tempC = dht.readTemperature();
        slocaltemp = (String)tempC;
        //Humi = dht.readHumidity();

//##### For testing purposes of DHT #####//
/*      Serial.println("---------------");
        Serial.print("Temperature Celsius = ");
        Serial.print(dht.readTemperature());
        Serial.println("C");
*/
//##### Readings to be added later #####//
/* 
        Serial.print("Temperature Fahrenheit = ");
        Serial.print(dht.readTemperature(false));
        Serial.println("F");

        Serial.print("Humidity = ");
        Serial.print(dht.readHumidity());
        Serial.println("%");

        Serial.print("HeatIndex = ");
        Serial.print(dht.readHeatIndex(tempC,Humi));
        Serial.println("C");

        Serial.print("DewPoint = ");
        Serial.print(dht.readDewPoint(tempC,Humi));
        Serial.println("C");
*/   }
    delay(2000);
  //---------- ./DHT Loop ----------//

  //---------- GPS Loop ----------//
//  Serial.println("----------------*");
//    if (getData(&info) > 3)   //Start retriving data if no of satellites being tracked are greater than 3
//    {
//      String str = "|";
//      str += date_format;
//      str += ",";
//      str += time_format;
//      str += ",";
//      str += lat_format;
//      str += ",";
//      str += lon_format;
//      str += ",";
//      str += sat_num;
//      str += "|";
//  //    Serial.println(str);
//    }
//    else
//    Serial.println("Less then 4 satelites.");
//    delay(pause);
//  //---------- ./GPS Loop ----------//

//******************************
  //---------- /ADXL345 Loop ----------//
      adxl.getAcceleration(xyz);
      
      ax = xyz[0];
      ay = xyz[1];
      az = xyz[2];
      
      tilt = ( ( (atan2(ay,az) * 180) / 3.14 ) + 180 );
    
    //########## For Testing Purposes ##########//
//      if(tilt >= 195){
//        rightTiltAlert();
//        return;
//      }
//      else if(tilt <= 165){
//        leftTiltAlert();
//        return;
//      }
//      else{
//        printValues();    
//      }  
    //########## ./For Testing Purposes ##########//
  //---------- ./ADXL345 Loop ----------//

  slocaltilt = (String)tilt;

  //---------- Light Sensor Loop --------//
      lightValue = analogRead(pinLight);
    
    //########## For Testing Purposes ##########//
//      if(lightValue < lightThresh){
//        Serial.print("\n\nAlert!!\nLight lesser than the Threshold value by ");
//        Serial.print(lightThresh - lightValue);
//        delay(1000);
//        return;
//      }
//    
//      Serial.print("\nValue of Light  : ");
//      Serial.print(lightValue);
//      Serial.println();
    //########## ./For Testing Purposes ##########//
  //---------- ./Light Sensor Loop --------//

    slocallight = (String)lightValue;
  
//******************************
//---------- Gas Sensor Loop --------//
int sensorValue=analogRead(A0);
float Resistance=(1023.0/sensorValue)-1;
delay(500);
double ppm= pow(10.0,((log10(Resistance/R0)-0.0827)/(-0.4807)));
slocalgas =(String)ppm;


//******************************    
    slocallat = (String)llatitude;
    slocallong = (String)llongitude;

    String payload = "{";
  payload += "\"Temperature\":\""; payload += slocaltemp; payload += "\", ";
  payload += "\"Latitude\":\""; payload += slocallat; payload += "\", ";
  payload += "\"Longitude\":\""; payload += slocallong; payload += "\", ";
  payload += "\"Tilt\":\""; payload += slocaltilt; payload += "\", ";
  payload += "\"Light\":\""; payload += slocallight; payload += "\",  ";
  payload += "\"GAS Sensor Value\":\""; payload += slocalgas; payload += "\"";  
  payload += "}";

      char analogData[10000];
      Serial.println(payload);
      payload.toCharArray( analogData, 10000 ); //copies the contents of the payload to the buffer 'analogData'
      client.publish( "sidsat", analogData ); //publishes to the topic sidsat
      Serial.println( analogData );
      Serial.println("Sending alert data");
      delay(500);
      
      uploadstatus();
}

//---------------------------- ADXL345 Functions -----------------------//
void printValues(){
  Serial.print("\nTilt Reading     :");
  Serial.print(tilt);
  Serial.println();
  delay(DELAY);
  
}

/=============== Alert Message Print Functions ===============/

void rightTiltAlert(){
  Serial.print("\n\nAlert!!\nContainer is tilted towards right by an angle of ");
  Serial.print(tilt - 180);
  Serial.print(" deg");
}

void leftTiltAlert(){
  Serial.print("\n\nAlert!!\nContainer is tilted towards left by an angle of ");
  Serial.print(180 - tilt);
  Serial.print(" deg");
}

/============== ./Alert Message Print Functions ==============/
//---------------------------- ./ADXL345 Functions -----------------


extern unsigned char getComma(unsigned char num,const char *str)
{
  unsigned char i,j = 0;
  int len=strlen(str);
  for(i = 0;i < len;i ++)
  {
     if(str[i] == ',')
      j++;
     if(j == num)
      return i + 1; 
  }
  return 0; 
}

extern double getDoubleNumber(const char *s)
{
  char buf[10];
  unsigned char i;
  double rev;
  
  i=getComma(1, s);
  i = i - 1;
  strncpy(buf, s, i);
  buf[i] = 0;
  rev=atof(buf);
  return rev; 
}

extern double getIntNumber(const char *s)
{
  char buf[10];
  unsigned char i;
  double rev;
  
  i=getComma(1, s);
  i = i - 1;
  strncpy(buf, s, i);
  buf[i] = 0;
  rev=atoi(buf);  
  return rev; 
}

char buff[256];


char buffer_latitude[8];
char buffer_longitude[8];

void parseGPGGA(const char* GPGGAstr)
{
 
  int tmp, hour, minute, second, num ;
  if(GPGGAstr[0] == '$')
  {
    tmp = getComma(1, GPGGAstr);
    hour     = (GPGGAstr[tmp + 0] - '0') * 10 + (GPGGAstr[tmp + 1] - '0');
    minute   = (GPGGAstr[tmp + 2] - '0') * 10 + (GPGGAstr[tmp + 3] - '0');
    second    = (GPGGAstr[tmp + 4] - '0') * 10 + (GPGGAstr[tmp + 5] - '0');
    
    sprintf(buff, "UTC timer %2d-%2d-%2d", hour, minute, second);
    //Serial.println(buff);
    
    tmp = getComma(2, GPGGAstr);
    llatitude = getDoubleNumber(&GPGGAstr[tmp])/100.0;
    int latitude_int=floor(llatitude);
    double latitude_decimal=(llatitude-latitude_int)*100.0/60.0;
    llatitude=latitude_int+latitude_decimal;
    tmp = getComma(4, GPGGAstr);
    llongitude = getDoubleNumber(&GPGGAstr[tmp])/100.0;
    int longitude_int=floor(llongitude);
    double longitude_decimal=(llongitude-longitude_int)*100.0/60.0;
    llongitude=longitude_int+longitude_decimal;
    
    sprintf(buff, "llatitude = %10.4f, llongitude = %10.4f", llatitude, llongitude);
    //Serial.println(buff); 
    
    tmp = getComma(7, GPGGAstr);
    num = getIntNumber(&GPGGAstr[tmp]);    
    sprintf(buff, "satellites number = %d", num);
    //Serial.println(buff); 
  }
  else
  {
    Serial.println("Not get data"); 
  }
  
}

void GPS_receive() {
  LGPS.getData(&info);
  //Serial.println((char*)info.GPGGA); 
  parseGPGGA((const char*)info.GPGGA);
}

#define SITE_URL "api.mediatek.com"
void uploadstatus()
{
while (!wifiClient.connect(SITE_URL, 80))
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Success!");
  delay(100);


wifiClient.println("POST /mcs/v2/devices/DIVJCnIK/datapoints.csv HTTP/1.1");
//#hint! Please do not add excessive spaces
if((llongitude >= (longtarget-longrange) && llongitude <= (longtarget+longrange))&& (llatitude >= (lattarget-latrange) && llatitude <= (lattarget+latrange)))
  {
    Serial.println("product arrived to detination");
  }
  else
  {
    Serial.println("product yet to arrive");
    }
 
float latitude_post=llatitude;
  float longitude_post=llongitude;
  Serial.printf("llatitude=%.4f\tllongitude=%.4f\n",llatitude,llongitude);
  if(llatitude>-90 && llatitude<=90 && llongitude>=0 && llongitude<360)
  {
    sprintf(buffer_latitude, "%.4f", llatitude);
    sprintf(buffer_longitude, "%.4f", llongitude);
  }
 
String data = "1,,"+ String(slocaltemp) + "\n" + "2,," + String(tilt)+ "\n" + "3,," + String(slocallight)+"\n"+"4,,"+String(buffer_latitude)+"\n"+"5,,"+String(buffer_longitude)+"\n"+ "6,," +String(buffer_latitude)+","+String(buffer_longitude)+","+"0";

//#hint! Please do not add excessive spaces
//#timestamp is in unix-time format the milliseconds and is optional. If not provided,
//#system will generate timestamp at the time of receiving this API
int dataLength = data.length();
wifiClient.println("Host: api.mediatek.com");
wifiClient.println("deviceKey: ojNs0ENyum5K30Rk");
wifiClient.print("Content-Length: ");
wifiClient.println(dataLength);
wifiClient.println("Content-Type: text/csv");
wifiClient.println("Connection: close");
wifiClient.println();
wifiClient.println(data);
 
  }