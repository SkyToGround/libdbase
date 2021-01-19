#include "DigiBASE.h"
#include "libdbasei.h"

std::vector<unsigned int> ListConnectedDetectors()
{
	std::vector<unsigned int> ret_vec;
	int list_of_serials[10];
	int serials_retreived = libdbase_list_serials(list_of_serials, 10);
	for (int i = 0; i < serials_retreived; i++)
	{
		ret_vec.push_back(list_of_serials[i]);
	}
	return ret_vec;
}

bool DigiBASE::DetectorIsOK()
{
	return is_ok;
}

DigiBASE::DigiBASE(unsigned int serial) : is_ok(false), listModeTime(0)
{
	det = libdbase_init(int(serial));
	pulseBuffer = new pulse[pulseBufferSize];
	if (det != NULL)
	{
		is_ok = true;
	}
}

DigiBASE::~DigiBASE()
{
	delete [] pulseBuffer;
	libdbase_close(det);
}

void DigiBASE::SetHVOn()
{
	if (is_ok)
	{
		int ret = libdbase_hv_on(det);
		if (-1 == ret)
		{
			is_ok = false;
		}
	}
}

void DigiBASE::GetEvents(std::vector<unsigned int> &amplitude, std::vector<unsigned int> &time) {
	int events;
	int res = libdbase_read_lm_packets(det, pulseBuffer, sizeof(pulse) * pulseBufferSize, &events, &listModeTime);
	for (int i = 0; i < events; i++) {
		amplitude.push_back(pulseBuffer[i].amp);
		time.push_back(pulseBuffer[i].time);
	}
}

void DigiBASE::SetPHAMode() {
	if (is_ok) {
		int ret = libdbase_set_pha_mode(det);
		if (-1 == ret) {
			is_ok = false;
		}
	}
}

void DigiBASE::SetListMode() {
	if (is_ok) {
		int ret = libdbase_set_list_mode(det);
		if (-1 == ret) {
			is_ok = false;
		}
	}
}

void DigiBASE::SetHVOff()
{
	if (is_ok)
	{
		int ret = libdbase_hv_off(det);
		if (-1 == ret)
		{
			is_ok = false;
		}
	}
}

void DigiBASE::StartMeasurement()
{
	if (is_ok)
	{
		int ret = libdbase_start(det);
		if (-1 == ret)
		{
			is_ok = false;
		}
	}
}

void DigiBASE::StopMeasurement()
{
	if (is_ok)
	{
		int ret = libdbase_stop(det);
		if (-1 == ret)
		{
			is_ok = false;
		}
	}
}

void DigiBASE::ClearSpectrum()
{
	if (is_ok)
	{
		int ret = libdbase_clear_spectrum(det);
		if (-1 == ret)
		{
			is_ok = false;
		}
	}
}

void DigiBASE::ClearTimers()
{
	if (is_ok)
	{
		int ret = libdbase_clear_counters(det);
		if (-1 == ret)
		{
			is_ok = false;
		}
	}
}

void DigiBASE::SetPulseWidth(float pulse_width)
{
	if (is_ok)
	{
		int ret = libdbase_set_pw(det, pulse_width);
		if (-1 == ret)
		{
			is_ok = false;
		}
	}
}

void DigiBASE::SetFineGain(float fine_gain)
{
	if (is_ok)
	{
		int ret = libdbase_set_fgn(det, fine_gain);
		if (-1 == ret)
		{
			is_ok = false;
		}
	}
}

void DigiBASE::SetHV(unsigned short high_voltage)
{
	if (is_ok)
	{
		int ret = libdbase_set_hv(det, high_voltage);
		if (-1 == ret)
		{
			is_ok = false;
		}
	}
}

std::map<std::string, std::string> DigiBASE::GetStatus()
{
	std::map<std::string, std::string> ret_map;
	
	if (is_ok)
	{
		int ret = libdbase_get_status(det);
		if (-1 == ret)
		{
			is_ok = false;
			return ret_map;
		}
	}
	else {
		return ret_map;
	}
	ret_map["high_voltage_target"] = std::to_string(det->status.HVT * HV_FACTOR);
	ret_map["fine_gain_setting"] = std::to_string(GET_INV_GAIN_VALUE(det->status.FGN));
	ret_map["fine_gain_actual"] = std::to_string(GET_INV_GAIN_VALUE(det->status.AFGN));
	ret_map["live_time"] = std::to_string(det->status.LT * TICKSTOSEC);
	ret_map["live_time_preset"] = std::to_string(det->status.LTP * TICKSTOSEC);
	ret_map["real_time"] = std::to_string(det->status.RT * TICKSTOSEC);
	ret_map["real_time_preset"] = std::to_string(det->status.RTP * TICKSTOSEC);
	ret_map["pulse_width"] = std::to_string(GET_PW(det->status.PW));
	
	ret_map["gs_upper"] = std::to_string(det->status.GSU);
	ret_map["gs_lower"] = std::to_string(det->status.GSL);
	ret_map["gs_center"] = std::to_string(det->status.GSC);
	
	if (det->status.CTRL & ON_MASK) {
		ret_map["measuring"] = "1";
	}
	else {
		ret_map["measuring"] = "0";
	}
	
	if (det->status.CTRL & MODE_MASK) {
		ret_map["detector_mode"] = "PHA";
	}
	else {
		ret_map["detector_mode"] = "List";
	}
	
	if (det->status.CTRL & HV_MASK) {
		ret_map["hv_on"] = "1";
	}
	else {
		ret_map["hv_on"] = "0";
	}
	
	if (det->status.CTRL & RTP_MASK) {
		ret_map["real_time_preset_on"] = "1";
	}
	else {
		ret_map["real_time_preset_on"] = "0";
	}
	
	if (det->status.CTRL & LTP_MASK) {
		ret_map["live_time_preset_on"] = "1";
	}
	else {
		ret_map["live_time_preset_on"] = "0";
	}
	
	if (det->status.CTRL & GS_MASK) {
		ret_map["gain_stab_on"] = "1";
	}
	else {
		ret_map["gain_stab_on"] = "0";
	}
	
	if (det->status.CTRL & ZS_MASK) {
		ret_map["zero_stab_on"] = "1";
	}
	else {
		ret_map["zero_stab_on"] = "0";
	}
	return ret_map;
}

std::vector<unsigned int> DigiBASE::GetSpectra()
{
	std::vector<unsigned int> ret_vec;
	
	if (is_ok)
	{
		int ret = libdbase_get_spectrum(det);
		if (-1 == ret)
		{
			is_ok = false;
			return ret_vec;
		}
	}
	else {
		return ret_vec;
	}
	
	for (int i = 0; i < 1024; i++) {
		ret_vec.push_back(det->spec[i]);
	}
	return ret_vec;
}

void DigiBASE::SetGSOn(bool status)
{
	if (is_ok)
	{
		int ret;
		if (status){
			ret = libdbase_gs_on(det);
		}
		else {
			ret = libdbase_gs_off(det);
		}
		if (-1 == ret)
		{
			is_ok = false;
		}
	}
}

void DigiBASE::SetZSOn(bool status)
{
	if (is_ok)
	{
		int ret;
		if (status){
			ret = libdbase_zs_on(det);
		}
		else {
			ret = libdbase_zs_off(det);
		}
		if (-1 == ret)
		{
			is_ok = false;
		}
	}
}


void DigiBASE::SetGSStats(unsigned short center, unsigned short width)
{
	det->status.GSC = center;
	det->status.GSL = center - width / 2;
	det->status.GSU = center + width / 2;
	int ret = libdbase_set_status(det);
	if (-1 == ret)
	{
		is_ok = false;
	}
}
