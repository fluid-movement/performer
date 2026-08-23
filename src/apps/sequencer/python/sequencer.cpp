#include "SequencerApp.h"
#include "model/Model.h"
#include "engine/QuantizerTrackEngine.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

void register_project(py::module &m);

void register_sequencer(py::module &m) {
    // ------------------------------------------------------------------------
    // Sequencer
    // ------------------------------------------------------------------------

    py::class_<SequencerApp> sequencer(m, "Sequencer");
    sequencer
        .def_property_readonly("model", [] (SequencerApp &app) { return &app.model; })
        // Quantizer loop-mode access for tests (engine-only state, not persisted)
        .def("quantizerLoopMode", [] (SequencerApp &app, int trackIdx) {
            return int(app.engine.trackEngine(trackIdx).as<QuantizerTrackEngine>().loopMode());
        })
        .def("setQuantizerLoopMode", [] (SequencerApp &app, int trackIdx, int mode) {
            app.engine.trackEngine(trackIdx).as<QuantizerTrackEngine>()
                .setLoopMode(QuantizerTrackEngine::LoopMode(mode));
        })
        .def("quantizerLoopFillCount", [] (SequencerApp &app, int trackIdx) {
            return app.engine.trackEngine(trackIdx).as<QuantizerTrackEngine>().loopFillCount();
        })
    ;

    // ------------------------------------------------------------------------
    // Model
    // ------------------------------------------------------------------------

    py::class_<Model> model(m, "Model");
    model
        .def_property_readonly("project", [] (Model &model) { return &model.project(); })
    ;

    register_project(m);
}
