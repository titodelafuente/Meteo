//
//    FILE: Meteo.ino
//  AUTHOR: Alberto
// VERSION: 0.4		16/01/2017
// PURPOSE: DHT Temperature & Humidity Sensor library for Arduino (2 sensors)
//			Timetag each measurement
// 				Messages consist of the letter T followed by ten digit time (as seconds since Jan 1 1970)
// 				http://www.onlineconversion.com/unix_time.htm
//				T1357041600 = 12:00 Jan 1 2013 
//			Logging into file (SD card)

#include <dht.h>
#include <TimeLib.h>
// include the SD library:
#include <SPI.h>
#include <SD.h>

dht DHT;
Sd2Card card;

const int DHT22_SENSOR1_PIN = 6;
const int DHT22_SENSOR2_PIN = 7;
const int TIME_REQUEST = 7;    // ASCII bell character requests a time sync message 
const int SD_CS_PIN = 13;


void setup() {
  String str;
  Serial.begin(115200);

  // SD Card
  int sd_status;
  sd_status = setup_SD_Card();
  
  // Header
  Serial.println("METEO PROGRAM ");
  Serial.print("LIBRARY VERSION: ");
  Serial.println(DHT_LIB_VERSION);
  Serial.println();
  
  // Sync
  setSyncProvider(requestSync);  //set function to call when sync required
  Serial.println("Waiting for sync message: ...");
  while(timeStatus() != timeSet){
    if (Serial.available()>0) {
      processSyncMessage();
    }
    str = "Sync Status: " + String(timeStatus());
    Serial.println(str);
	  delay(2000);
  }
  str = "Sync valid: " + logDate(now()) + " " + logTime(now());
  Serial.println(str);
  Serial.println("\nSensor,\t\tDay,\t\tTime,\t\tStatus,\tHumidity (%),\tTemp (C)");
}

//*********************************************************
void loop()
{
	// sensor1
	int chk = DHT.read22(DHT22_SENSOR1_PIN);
	String str1 = logData("DHT22_S1", now(), chk, DHT);
	Serial.println(str1);
 
	// sensor2
	chk = DHT.read22(DHT22_SENSOR2_PIN);
	String str2 = logData("DHT22_S2", now(), chk, DHT);
	Serial.println(str2);
  
	delay(15000);
}

//*********************************************************
String logData(char id[], time_t t, int check, dht sensor) {
	String sep = ", \t";	// separator
	String ch;
	switch(check) {
	case DHTLIB_OK:  
		ch = "OK"; 
		break;
	case DHTLIB_ERROR_CHECKSUM: 
		ch = "Checksum_error"; 
		break;
	case DHTLIB_ERROR_TIMEOUT: 
		ch = "Time_out_error"; 
		break;
	default: 
		ch = "Unknown_error"; 
		break;
	}
	String str = id + sep + logDate(t) + sep + logTime(t) + sep + ch + sep + sensor.humidity + sep + sensor.temperature;
	return str;
}

//*********************************************************
time_t requestSync() {
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}

//*********************************************************
void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013
  const char time_header='T';   // Header tag for serial time sync message

  if(Serial.find(time_header)) {
     pctime = Serial.parseInt();
     if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
       setTime(pctime); // Sync Arduino clock to the time received on the serial port
    }
  }else {
    setTime(DEFAULT_TIME);
  }
}

//*********************************************************
String logDate(time_t t){
	// digital clock display of the time: yyyy-mm-dd
	String str;
	str = String(year(t)) + "-" + printDigits(month(t)) + "-" + printDigits(day(t));
	return str;
}

//*********************************************************
String logTime(time_t t){
	// digital clock display of the time: hh:mm:ss
	String str;
	str = printDigits(hour(t)) + ":" + printDigits(minute(t)) + ":" + printDigits(second(t));
	return str;
}

//*********************************************************
String printDigits(int digits){
	// utility function for digital clock display: prints leading 0
	String str;
	if(digits < 10)
		str = "0";
	str += digits;
	return str;
}

//*********************************************************
int setup_SD_Card() {
	Serial.print("\nInitializing SD card... ");

	// testing if the card is working
	if (!card.init(SPI_HALF_SPEED, SD_CS_PIN)) {
		Serial.println("failed.");
		return -1;
	}else {
		Serial.println("wiring is correct and a card is present.");
	}

	// print the type of card
	String str, strType;
	switch (card.type()) {
		case SD_CARD_TYPE_SD1:
			strType = "SD1";
			break;
		case SD_CARD_TYPE_SD2:
			strType = "SD2";
			break;
		case SD_CARD_TYPE_SDHC:
			strType = "SDHC";
			break;
		default:
			strType = "Unknown";
	}
	str = "Card type: " + strType;
	Serial.println(str);

	return 0;
}
//
// END OF FILE
//
