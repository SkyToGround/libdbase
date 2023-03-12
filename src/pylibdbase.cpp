#include "DigiBASE.h"
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(pylibdbase, m) {
    py::class_<DigiBASE>(m, "DigiBASE")
        .def(py::init<unsigned int>())
        .def("SetHVOn", &DigiBASE::SetHVOn, py::arg("On") = true)
        .def("SetHVOff", &DigiBASE::SetHVOff)
				.def("StartMeasurement", &DigiBASE::StartMeasurement)
				.def("StopMeasurement", &DigiBASE::StopMeasurement)
				.def("SetPulseWidth", &DigiBASE::SetPulseWidth)
				.def("SetFineGain", &DigiBASE::SetFineGain)
				.def("SetHV", &DigiBASE::SetHV)
				.def("SetZSOn", &DigiBASE::SetZSOn)
				.def("SetGSOn", &DigiBASE::SetGSOn)
				.def("ClearSpectrum", &DigiBASE::ClearSpectrum)
				.def("ClearTimers", &DigiBASE::ClearTimers)
				.def("SetGSStats", &DigiBASE::SetGSStats)
				.def("GetStatus", &DigiBASE::GetStatus)
				.def("GetSpectra", &DigiBASE::GetSpectra)
				.def("DetectorIsOK", &DigiBASE::DetectorIsOK);
		
		m.def("ListConnectedDetectors", &ListConnectedDetectors, "List available detectors");
}
