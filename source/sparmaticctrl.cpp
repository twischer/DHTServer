#include "sparmaticctrl.h"
#include <EEPROM.h>

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#define DELAY_IN_MS			150
#define MIN_TEMPERATURE		7
#define MAX_TEMPERATURE		30

#define TEMP_ADDRESS		0


SparmaticCtrl::SparmaticCtrl(const uint8_t pin1, const uint8_t pin2) :
	_pin1(pin1),
	_pin2(pin2),
	_currentTemperature(0),
	_lastOnTemperature(20)
{
}


void SparmaticCtrl::begin(void)
{
	/* use inverted logic, because of PNP transistor */
	/* simulate open state for shaft encoder */
	digitalWrite(_pin1, HIGH);
	digitalWrite(_pin2, HIGH);
	pinMode(_pin1, OUTPUT);
	pinMode(_pin2, OUTPUT);

	/* switch sparmatic off after reset
	 * is needed to deteckt the correct offset
	 */
	off();

	EEPROM.begin(4);
	const uint8_t lastTemp = EEPROM.read(TEMP_ADDRESS);

	/* onyl restore the temperature,
	 * if it is in an valid range
	 */
	if (lastTemp >= MIN_TEMPERATURE && lastTemp <= MAX_TEMPERATURE) {
		setTemp(lastTemp);
		Serial.print("Temperature: ");
		Serial.print(lastTemp);
		Serial.print(" restored\n");
	} else {
		Serial.print("Invalid temperature: ");
		Serial.print(lastTemp);
		Serial.print(" Not restoring!\n");
	}
}


void SparmaticCtrl::_shaftEncoderStep(const bool up)
{
	uint8_t pin1 = _pin1;
	uint8_t pin2 = _pin2;
	if (!up) {
		pin1 = _pin2;
		pin2 = _pin1;
	}

	digitalWrite(pin1, LOW);
	delay(DELAY_IN_MS);
	digitalWrite(pin2, LOW);
	delay(DELAY_IN_MS);
	digitalWrite(pin1, HIGH);
	delay(DELAY_IN_MS);
	digitalWrite(pin2, HIGH);
	delay(DELAY_IN_MS);
}


void SparmaticCtrl::setTemp(const uint8_t temperature)
{
	/* switch off, if temperature is to small */
	if (temperature < MIN_TEMPERATURE) {
		off();
		return;
	}

	const bool incTemp = (temperature > _currentTemperature);
	uint8_t diff = 0;
	if (incTemp) {
		diff = temperature - _currentTemperature;
	} else {
		diff = _currentTemperature - temperature;
	}

	/*
	 * remove offset,
	 * if diff includes switch on temperature (MIN_TEMPERATURE).
	 * The value of _currentTemperature always has to be between
	 * MIN_TEMPERATURE and MAX_TEMPERATURE or 0
	 */
	if (_currentTemperature < MIN_TEMPERATURE) {
		diff -= MIN_TEMPERATURE;
	}

	for (uint8_t i=0; i<diff; i++) {
		_shaftEncoderStep(incTemp);
	}

	_saveTemp(temperature);
}


void SparmaticCtrl::_saveTemp(const uint8_t newTemp)
{
	if (newTemp < MIN_TEMPERATURE && newTemp > 0) {
		_currentTemperature = MIN_TEMPERATURE;
	} else if (newTemp > MAX_TEMPERATURE){
		_currentTemperature = MAX_TEMPERATURE;
	} else {
		_currentTemperature = newTemp;
	}

	Serial.print("Saving temperature: ");
	Serial.print(_currentTemperature);
	Serial.print(" to EEPROM\n");

	/* save the current temperature
	 * to restore it on a restart
	 */
	EEPROM.write(TEMP_ADDRESS, _currentTemperature);
	if (!EEPROM.commit()) {
		Serial.print("Saving failed!\n");
	}
}


void SparmaticCtrl::incTemp(void)
{
	_shaftEncoderStep(true);
	_saveTemp(_currentTemperature + 1);
}


void SparmaticCtrl::decTemp(void)
{
	_shaftEncoderStep(false);
	_saveTemp(_currentTemperature - 1);
}


void SparmaticCtrl::off()
{
	/* save current temperature for switching on again */
	_lastOnTemperature = _currentTemperature;

	/*
	 * switch fully off to reset possibly temperature offsets
	 * between Sparmatic and _currentTemperature
	 */
	for (uint8_t i=0; i<MAX_TEMPERATURE; i++) {
		_shaftEncoderStep(false);
	}

	_currentTemperature = 0;
}


void SparmaticCtrl::restore()
{
	setTemp(_lastOnTemperature);
}
