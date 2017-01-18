//
//    FILE: Meteo.ino
//  AUTHOR: Alberto
// VERSION: 0.4		10/02/2017
// PURPOSE: DHT Temperature & Humidity Sensor library for Arduino (2 sensors)
//			Timetag each measurement
// 				Messages consist of the letter T followed by ten digit time (as seconds since Jan 1 1970)
// 				http://www.onlineconversion.com/unix_time.htm
//				E.g.  T1357041600 = 12:00 Jan 1 2013 
//			Logging into daily file (SD card). Flush to file every hour.

#include <dht.h>
#include <TimeLib.h>
// include the SD library:
#include <SPI.h>
#include <SD.h>


const int DHT22_SENSOR1_PIN = 6;
const int DHT22_SENSOR2_PIN = 7;
const int TIME_REQUEST = 7;    // ASCII bell character requests a time sync message 
const int SD_CS_PIN = 10;

void setup() {
  Serial.begin(9600);

  // Header
  String str = "METEO PROGRAM\nLIBRARY VERSION: " + String(DHT_LIB_VERSION) + "\n";
  Serial.println(str);
  
  // SD Card
  if (setup_SD_Card() != 0 )
    return;

  // Time Sync
  setup_Time(); 
}

//*********************************************************
void loop() {
  static File fich;
  char str[100];
  time_t epoch = now();

  // manage file
  openFile(&fich, epoch);

	// measurement sensor1
  measureData("DHT22_S1", epoch, DHT22_SENSOR1_PIN, str);
  writeFile(fich, str);
  
	// measurement sensor2
  measureData("DHT22_S2", epoch, DHT22_SENSOR2_PIN, str);
  writeFile(fich, str);
  
	delay(60000); // every 60"
}

//*********************************************************
void measureData(const char id[], const time_t t, const int pin, char str[]) {
	dht dhtsensor;
  char strDate[15], strTime[15];
	String ch;
  int check = dhtsensor.read22(pin);

	switch(check) {
  	case DHTLIB_OK:  
  		ch = "OK"; 
  		break;
  	case DHTLIB_ERROR_CHECKSUM: 
  		ch = "Checksum_error"; 
  		break;
  	case DHTLIB_ERROR_TIMEOUT: 
  		ch = "TimeOut_error"; 
  		break;
  	default: 
  		ch = "Unknown_error"; 
  		break;
	}
  logEpoch(t, strDate, strTime);
  sprintf(str, "%s,\t%s,\t%s,\t%s,\t%s,\t\t%s\n", id, strDate, strTime, ch.c_str(), String(dhtsensor.humidity).c_str(), String(dhtsensor.temperature).c_str() );
}

//*********************************************************
// Create new file every day
void openFile(File *f, const time_t t) {
  static int day_previous = 0;
  static int hour_previous = 0;
  static char filename[15];
  
  if( day(t) != day_previous ){
    day_previous = day(t);
    hour_previous = hour(t);
    
    // close previous file
    f->flush();    
    f->close();
    
    // open new file metMMDD.0YY
    String yy = String(year(t));
    sprintf(filename,"met%02d%02d.0%s", month(t), day(t), yy.substring(2).c_str());
    Serial.println(filename);
    *f = SD.open(filename, FILE_WRITE);

    char header[] = "Sensor,\t\tDate,\t\tTime,\t\tStatus,\tHumidity (%),\tTemp (C)\n";
    writeFile(*f, header);
  }else{
    if( hour(t) != hour_previous ){
      // save data to file every hour
      hour_previous = hour(t);
      f->flush();
      f->close();
      *f = SD.open(filename, FILE_WRITE);   // append
      Serial.println("Data saved to file in SD card.");  
    }
  }
}

//*********************************************************
void writeFile(const File f, const char str[]) {
  Serial.print(str);
  if (f) {
    f.write(str);
  }else{
    Serial.print("ERROR: File not opened.\n");
  }
}

//*********************************************************
time_t requestSync() {
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}

//*********************************************************
void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1483228800; // Jan 1 2017
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
void logEpoch(const time_t t, char strTime[], char strDate[]){
  // digital clock display of the time: yyyy-mm-dd
  sprintf(strTime, "%04d-%02d-%02d", year(t), month(t), day(t));
  // digital clock display of the time: HHhMM:SS
  sprintf(strDate, "%02dh%02d:%02d", hour(t), minute(t), second(t));
}

//*********************************************************
int setup_SD_Card() {
	Sd2Card card;

	// testing if the card is working
	if (!card.init(SPI_HALF_SPEED, SD_CS_PIN)) {
		Serial.println("ERROR: Initializing SD card.");
		return -1;
	} else {
		Serial.println("Initializing SD card. Wiring is correct and a card is present.");
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
	str = "Card type: " + strType + "\n";

  if (!SD.begin(SD_CS_PIN)) {
    str += "ERROR: Initialization SD failed.\n";
    return -1;
  }else{
    str += "Initialization SD done.\n";
  }
	Serial.println(str);

	return 0;
}

//*********************************************************
void setup_Time() {
  String str;
  char strDate[15], strTime[15];
  int cont = 0; 
  setSyncProvider(requestSync);  //set function to call when sync required
  
  Serial.println("Waiting for sync message: ...");
  while(timeStatus() != timeSet){
    // if there is an input in serial port or after 10 iterations
    if (Serial.available()>0 || cont==10) {
      processSyncMessage();
    }
    
    str = "Sync Status: " + String(timeStatus());
    Serial.println(str);
    cont++;
    delay(2000);
  }
  logEpoch(now(), strDate, strTime);
  str = "Sync valid: " + String(strDate) + " " + String(strTime);
  Serial.println(str);
}

//
// END OF FILE
//
