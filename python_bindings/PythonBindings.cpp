/*
 * Copyright 2023 Robert Bezem (robert@sqrtroot.com)
 * SPDX-License-Identifier: Apache-2.0
*/
#include "ScsiDriver.hpp"
#include "IT8951.hpp"
#include "ScreenManager.hpp"
#include "log.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

ScreenManager create_screenmanager(const char *path, double vcom) {
    return {IT8951(ScsiDriver(path))};
}

PYBIND11_MODULE(IT8951, m) {
    //ScreenManager
    py::class_<ScreenManager>(m, "ScreenManager")
            .def("display", &ScreenManager::display)
            .def("clear_screen", &ScreenManager::clear_screen)
            .def("set_vcom", &ScreenManager::set_vcom)
            .def("set_rotation", &ScreenManager::set_rotation);

    m.def("create_screenmanager", &create_screenmanager, py::arg("path"), py::arg("vcom"),
          py::return_value_policy::move);

    //Logging
    py::enum_<LogLevel>(m, "LogLevel")
            .value("Debug", LogLevel::Debug)
            .value("Info", LogLevel::Info)
            .value("Warning", LogLevel::Warning)
            .value("Error", LogLevel::Error);

    m.def("setLogLevel",[](LogLevel logLevel){maxLogLevel=logLevel;}, py::arg("logLevel"));

//    m.attr("maxLogLevel") = py::cast(maxLogLevel, pybind11::return_value_policy::reference);
}

namespace PYBIND11_NAMESPACE {
    namespace detail {
        template<>
        struct type_caster<std::filesystem::path> {
        public:
            /**
             * This macro establishes the name 'inty' in
             * function signatures and declares a local variable
             * 'value' of type inty
             */
        PYBIND11_TYPE_CASTER(std::filesystem::path, const_name("std::filesystem::path"));

            /**
             * Conversion part 1 (Python->C++): convert a PyObject into a inty
             * instance or return false upon failure. The second argument
             * indicates whether implicit conversions should be applied.
             */
            bool load(handle src, bool) {
                type_caster<std::string> string_caster;
                bool result = string_caster.load(src, false); // input bool is ignored
                if(!result) {
                    log(LogLevel::Error, "Failed to convert to string");
                    return false;
                }
                value = std::filesystem::path((std::string)string_caster);
                return true;
            }

            /**
             * Conversion part 2 (C++ -> Python): convert an inty instance into
             * a Python object. The second and third arguments are used to
             * indicate the return value policy and parent object (for
             * ``return_value_policy::reference_internal``) and are generally
             * ignored by implicit casters.
             */
            static handle cast(std::filesystem::path src, return_value_policy /* policy */, handle /* parent */) {
                return PyBytes_FromString(src.c_str());
            }
        };
    }

} // namespace PYBIND11_NAMESPACE::detail