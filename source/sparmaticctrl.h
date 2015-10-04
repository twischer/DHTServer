#ifndef SPARMATICCTRL_H
#define SPARMATICCTRL_H

#include <stdint.h>

class SparmaticCtrl
{
private:
	const uint8_t _pin1;
	const uint8_t _pin2;
	uint8_t _currentTemperature;
	uint8_t _lastOnTemperature;

	void _shaftEncoderStep(const bool up);
	void _saveTemp(const uint8_t newTemp);

public:
	SparmaticCtrl(const uint8_t pin1, const uint8_t pin2);
	void begin(void);
	void setTemp(const uint8_t temperature);
	void incTemp(void);
	void decTemp(void);
	void off(void);
	void restore(void);

	uint8_t currentSetTemperature(void) { return _currentTemperature; }
};

#endif // SPARMATICCTRL_H
