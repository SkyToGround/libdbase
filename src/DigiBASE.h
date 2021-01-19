#pragma once

#include <vector>
#include <map>
#include <string>
#include "libdbase.h"

std::vector<unsigned int> ListConnectedDetectors();

class DigiBASE {	
public:
	DigiBASE(unsigned int serial);
	~DigiBASE();
	void SetHVOn();
	void SetHVOff();
	void StartMeasurement();
	void StopMeasurement();
	void SetPulseWidth(float pulse_width);
	void SetFineGain(float fine_gain);
	void SetHV(unsigned short high_voltage);
	void SetZSOn(bool status);
	void SetGSOn(bool status);
	void ClearSpectrum();
	void ClearTimers();
	void SetPHAMode();
	void SetListMode();
	void GetEvents(std::vector<unsigned int> &amplitude, std::vector<unsigned int> &time);
	void SetGSStats(unsigned short center, unsigned short width);
	std::map<std::string, std::string> GetStatus();
	std::vector<unsigned int> GetSpectra();
	bool DetectorIsOK();
private:
	const int pulseBufferSize = 4096;
	unsigned int listModeTime;
	bool is_ok;
	detector *det;
	pulse *pulseBuffer;
};
