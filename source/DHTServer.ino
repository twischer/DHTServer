/* DHTServer - ESP8266 Webserver with a DHT sensor as an input
 * 
 *  Based on ESP8266Webserver, DHTexample, and BlinkWithoutDelay (thank you)
 * 
 *  Version 1.0  5/3/2014  Version 1.0   Mike Barela for Adafruit Industries
 */
#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include "config.h"
#include "sparmaticctrl.h"

#define DHTTYPE DHT22
#define DHTPIN  3	// is RxD (TODO try with 2 on failes)

const char ssid[] = SSID;
const char password[] = PASSWORD;

#define SHAFT_ENCODER_PIN1	0
#define SHAFT_ENCODER_PIN2	2


const String rootPage =
		"<html>"
		"<body>"
//		"<p><a href=\"/csv\">CSV formatted status</a></p>"
		"<p><a href=\"/thermostat/set?temp=20\">Set 20 &deg;C</a></p>"
		"<p><a href=\"/thermostat/inc\">Inc temperature</a></p>"
		"<p><a href=\"/thermostat/dec\">Dec temperature</a></p>"
		"<p><a href=\"/thermostat/off\">Switch thermostat off</a></p>"
		"<p><a href=\"/thermostat/restore\">Restore last thermostat temperature</a></p>"
		"</body>"
		"</html>";

//const String okPage =
//		"<html>"
//		"<head>"
//		"<meta http-equiv=\"refresh\" content=\"2;url=/\">"
//		"</head>"
//		"<body>"
//		"<p>Command executed. Return to home.</p>"
//		"</body>"
//		"</html>";

const char pageType[] = "text/html";
ESP8266WebServer server(80);

// Initialize DHT sensor 
// NOTE: For working with a faster than ATmega328p 16 MHz Arduino chip, like an ESP8266,
// you need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold.  It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value.  The default for a 16mhz AVR is a value of 6.  For an
// Arduino Due that runs at 84mhz a value of 30 works.
// This is for the ESP8266 processor on ESP-01 
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266

SparmaticCtrl thermostat(SHAFT_ENCODER_PIN1, SHAFT_ENCODER_PIN2);

float humidity = 0.0;
float temperatureActual = 0.0;  // Values read from sensor

String webString="";     // String to display
// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 2000;              // interval at which to read sensor


static String getCSVString(void)
{
	/*
	 * use CSV style
	 * 	Set temp (in °C);Actual temp (in °C);Humidity (in %);Window
	 */
	String csvLine = String(thermostat.currentSetTemperature()) + ";";
	// TODO try String(temperatureActual, 1)
	csvLine += String((int)temperatureActual) + ";";
	csvLine += String((int)humidity) + ";";

	// TODO check reed contacts for window state
	csvLine += "close";

	return csvLine;
}


static void sendRoot()
{
	const String page = rootPage + getCSVString();
	server.send(200, pageType, page);
}


static void sendCSV()
{
	gettemperature();

	const String webString = getCSVString();
	server.send(200, "text/plain", webString);
}


void setup(void)
{
	// You can open the Arduino IDE Serial Monitor window to see what the code is doing
	Serial.begin(115200);  // Serial connection from ESP-01 via 3.3v console cable
	thermostat.begin();
	dht.begin();           // initialize temperature sensor
	
	// Connect to WiFi network
	WiFi.begin(ssid, password);
	Serial.print("\n\r \n\rWorking to connect");
	
	// Wait for connection
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.println("DHT Weather Reading Server");
	Serial.print("Connected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
	
	server.on("/", sendRoot);
	server.on("/csv", sendCSV);


	server.on("/thermostat/set", [](){
		const String temp = server.arg("temp");
		thermostat.setTemp(temp.toInt());
		sendRoot();
	});

	server.on("/thermostat/inc", [](){
		thermostat.incTemp();
		sendRoot();
	});

	server.on("/thermostat/dec", [](){
		thermostat.decTemp();
		sendRoot();
	});

	server.on("/thermostat/off", [](){
		thermostat.off();
		sendRoot();
	});
	
	server.on("/thermostat/restore", [](){
		thermostat.restore();
		sendRoot();
	});


	server.on("/temp", [](){  // if you add this subdirectory to your webserver call, you get text below :)
		gettemperature();       // read sensor
		webString="Temperature: "+String((int)temperatureActual)+" C";
		server.send(200, "text/plain", webString);            // send to someones browser when asked
	});
	
	server.on("/humidity", [](){  // if you add this subdirectory to your webserver call, you get text below :)
		gettemperature();           // read sensor
		webString="Humidity: "+String((int)humidity)+"%";
		server.send(200, "text/plain", webString);               // send to someones browser when asked
	});
	
	server.begin();
	Serial.println("HTTP server started");
}


void loop(void)
{
	server.handleClient();
} 


void gettemperature() {
	// Wait at least 2 seconds between measurements.
	// if the difference between the current time and last time you read
	// the sensor is bigger than the interval you set, read the sensor
	// Works better than delay for things happening elsewhere also
	unsigned long currentMillis = millis();
	
	if(currentMillis - previousMillis >= interval) {
		// save the last time you read the sensor 
		previousMillis = currentMillis;   
		
		// Reading temperature for humidity takes about 250 milliseconds!
		// Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
		humidity = dht.readHumidity();          // Read humidity (percent)
		temperatureActual = dht.readTemperature(false);     // Read temperature as Celsius
		// Check if any reads failed and exit early (to try again).
		if (isnan(humidity) || isnan(temperatureActual)) {
			Serial.println("Failed to read from DHT sensor!");
			return;
		}
	}
}
