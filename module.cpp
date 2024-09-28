#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#include <Windows.h>
#include <cmath>

namespace py = pybind11;

py::array_t<double> return_numpy_array() {
    // Create a NumPy array
    py::array_t<double> result({ 3, 3 });
    auto buffer = result.request();
    double* ptr = static_cast<double*>(buffer.ptr);

    // Fill the array with values
    for (size_t i = 0; i < 9; i++) {
        ptr[i] = static_cast<double>(i);
    }

    return result;
}

PYBIND11_MODULE(fasttlogparser, m) {
    m.def("return_numpy_array", &return_numpy_array);
#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}