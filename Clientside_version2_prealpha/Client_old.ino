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
  
  if (tp=='1')
  {
   //---------- DHT Loop ----------//
      if(dht.read())
      {   
        Serial.println("Statring to read temperature");
        tempC = dht.readTemperature();
        //Humi = dht.readHumidity();

//##### For testing purposes of DHT #####//
/*        Serial.println("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
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
//  Serial.println("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
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
  //    Serial.println(str);
    }
    else{
      Serial.println("Less then 4 satelites.");
    }
    delay(pause);
  //---------- ./GPS Loop ----------//
    
    data_A1 = (String)latitude;
      data_A2 = (String)longitude;
    
    //data_A1 = "0.00";
    //data_A2 = "0.00";
   }
   else
   {
     LGPS.powerOff();
     data_A1 = "f";
    data_A2 = "f";
   }

//****************************************************************************************

if(tS == '1'){
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
//****************************************************************************************

  // Just debug messages
  Serial.print( "Sending analog data : [" );
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
    Serial.println("Memory Data Entered");
    dataFile.print(payload);
    dataFile.close();
    Serial.println(payload);
  }
  
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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
//'''''''''' ./GPS Functions ''''''''''//





//--------------------------------------------------------------------------
void reportdata() 
{
  Serial.println("Sensing data for threshold \n\n");
  if(dht.read())
  {   
        Serial.println("Starting to read temperature");
        tempC = dht.readTemperature();
        localtemp = tempC;
        //Humi = dht.readHumidity();

//##### For testing purposes of DHT #####//
/*      Serial.println("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
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
//  Serial.println("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
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
  //    Serial.println(str);
    }
    else
    Serial.println("Less then 4 satelites.");
    delay(pause);
  //---------- ./GPS Loop ----------//
    
    locallat = latitude;
    locallong = longitude;

//****************************************************************************************
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
//****************************************************************************************
}


//--------------------------------------------------------------------------
void sendreportdata() 
{
  Serial.println( "Sensing data for threshold \n\n");
  if(dht.read())
  {   
        Serial.println("Starting to read temperature");
        tempC = dht.readTemperature();
        slocaltemp = (String)tempC;
        //Humi = dht.readHumidity();

//##### For testing purposes of DHT #####//
/*      Serial.println("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
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
//  Serial.println("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
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
  //    Serial.println(str);
    }
    else
    Serial.println("Less then 4 satelites.");
    delay(pause);
  //---------- ./GPS Loop ----------//

//****************************************************************************************
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
  
//****************************************************************************************
    
    slocallat = (String)latitude;
    slocallong = (String)longitude;

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
      Serial.println("Sending alert data");
      delay(500);
}

//---------------------------- ADXL345 Functions -----------------------//
void printValues(){
  Serial.print("\nTilt Reading     :");
  Serial.print(tilt);
  Serial.println();
  delay(DELAY);
}

/*=============== Alert Message Print Functions ===============*/

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

/*============== ./Alert Message Print Functions ==============*/
//---------------------------- ./ADXL345 Functions -----------------------//
