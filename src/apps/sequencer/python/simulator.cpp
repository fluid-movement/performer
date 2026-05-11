#include "sim/Simulator.h"
#include "sim/TargetState.h"
#include "sim/TargetUtils.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <vector>

namespace py = pybind11;

using namespace sim;

void register_simulator(py::module &m) {
    // ------------------------------------------------------------------------
    // TargetState sub-structs
    // ------------------------------------------------------------------------

    py::class_<ButtonState>(m, "ButtonState")
        .def("__getitem__", [](const ButtonState &s, int i) { return s.state.test(i); })
        .def("__len__", [](const ButtonState &) { return ButtonState::Count; })
        .def("to_list", [](const ButtonState &s) {
            std::vector<bool> v(ButtonState::Count);
            for (int i = 0; i < ButtonState::Count; ++i) v[i] = s.state.test(i);
            return v;
        });

    py::class_<AdcState>(m, "AdcState")
        .def("__getitem__", [](const AdcState &s, int i) { return s.state[i]; })
        .def("__len__", [](const AdcState &) { return AdcState::Count; })
        .def("to_list", [](const AdcState &s) {
            return std::vector<uint16_t>(s.state.begin(), s.state.end());
        });

    py::class_<DigitalInputState>(m, "DigitalInputState")
        .def("__getitem__", [](const DigitalInputState &s, int i) { return s.state.test(i); })
        .def("__len__", [](const DigitalInputState &) { return DigitalInputState::Count; });

    py::class_<LedState>(m, "LedState")
        .def("get", [](const LedState &s, int i) -> py::tuple {
            return py::make_tuple(s.state.test(i * 2), s.state.test(i * 2 + 1));
        }, "Returns (red, green) for LED at index i")
        .def("__len__", [](const LedState &) { return LedState::Count; })
        .def("to_list", [](const LedState &s) {
            std::vector<py::tuple> v;
            v.reserve(LedState::Count);
            for (int i = 0; i < LedState::Count; ++i)
                v.push_back(py::make_tuple(s.state.test(i * 2), s.state.test(i * 2 + 1)));
            return v;
        });

    py::class_<GateOutputState>(m, "GateOutputState")
        .def("__getitem__", [](const GateOutputState &s, int i) { return s.state.test(i); })
        .def("__len__", [](const GateOutputState &) { return GateOutputState::Count; })
        .def("to_list", [](const GateOutputState &s) {
            std::vector<bool> v(GateOutputState::Count);
            for (int i = 0; i < GateOutputState::Count; ++i) v[i] = s.state.test(i);
            return v;
        });

    py::class_<DacState>(m, "DacState")
        .def("__getitem__", [](const DacState &s, int i) { return s.state[i]; })
        .def("__len__", [](const DacState &) { return DacState::Count; })
        .def("to_list", [](const DacState &s) {
            return std::vector<uint16_t>(s.state.begin(), s.state.end());
        })
        .def("volts", [](const DacState &s, int i) { return dacToVoltage(s.state[i]); },
             "Convert DAC code at channel i to volts (1V/oct, 0V = C4)");

    py::class_<DigitalOutputState>(m, "DigitalOutputState")
        .def("__getitem__", [](const DigitalOutputState &s, int i) { return s.state.test(i); })
        .def("__len__", [](const DigitalOutputState &) { return DigitalOutputState::Count; });

    py::class_<LcdState>(m, "LcdState")
        .def("bytes", [](const LcdState &s) {
            return py::bytes(reinterpret_cast<const char *>(s.state.data()), s.state.size());
        }, "Raw LCD framebuffer as bytes (256*64, each byte = pixel brightness 0-255)")
        .def("width", [](const LcdState &) { return TargetConfig::LcdWidth; })
        .def("height", [](const LcdState &) { return TargetConfig::LcdHeight; });

    py::class_<TargetState>(m, "TargetState")
        .def_readonly("button", &TargetState::button)
        .def_readonly("adc", &TargetState::adc)
        .def_readonly("digitalInput", &TargetState::digitalInput)
        .def_readonly("led", &TargetState::led)
        .def_readonly("gateOutput", &TargetState::gateOutput)
        .def_readonly("dac", &TargetState::dac)
        .def_readonly("digitalOutput", &TargetState::digitalOutput)
        .def_readonly("lcd", &TargetState::lcd);

    // ------------------------------------------------------------------------
    // Simulator
    // ------------------------------------------------------------------------

    py::class_<Simulator> simulator(m, "Simulator", py::dynamic_attr());
    simulator
        .def("wait", &Simulator::wait)
        .def("setButton", &Simulator::setButton)
        .def("setEncoder", &Simulator::setEncoder)
        .def("rotateEncoder", &Simulator::rotateEncoder)
        .def("setAdc", &Simulator::setAdc)
        .def("setDio", &Simulator::setDio)
        .def("sendMidi", &Simulator::sendMidi)
        .def("screenshot", &Simulator::screenshot)
        .def_property_readonly("targetState", &Simulator::targetState, py::return_value_policy::reference)
    ;

    // ------------------------------------------------------------------------
    // TargetTrace
    // ------------------------------------------------------------------------

    py::class_<TargetTrace> trace(m, "TargetTrace", py::dynamic_attr());
    trace
        .def(py::init<>())

        .def("saveToFile", &TargetTrace::saveToFile)
        .def("loadFromFile", &TargetTrace::loadFromFile)
        .def("saveToText", &TargetTrace::saveToText)
    ;
}
