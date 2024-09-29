#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#define MAVLINK_USE_MESSAGE_INFO
#include <ardupilotmega/mavlink.h>
#include <mavlink_helpers.h>

#include "MessageSeries.h"

namespace py = pybind11;

using MavId = std::pair<uint8_t, uint8_t>;

void parseTLog(const std::string &path,
               std::vector<MavId> ids = {},
               std::vector<std::string> whitelist = {},
               std::vector<std::string> backlist = {})
{
    mavlink_message_t *msg;
    // std::map<uint32_t, MessageSeries::Shr> series_map;

    // const char *data;
    // uint64_t offset;

    // uint32_t msgid = msg->msgid;
    // auto pair = series_map.find(msgid);
    // if (pair != series_map.end())
    // {
    //     pair->second->addOffsets(offset);
    // }
    // else
    // {
    //     const mavlink_message_info_t *msg_info = mavlink_get_message_info(msg);
    //     auto series = std::make_shared<MessageSeries>(msg_info, data);
    //     series->addOffsets(offset);
    //     series_map.insert({msgid, series});
    // }
}

template <typename T>
std::vector<T> make_vector(std::size_t size)
{
    std::vector<T> v(size, 0);
    std::iota(v.begin(), v.end(), 0);
    return v;
}

template <typename Sequence>
inline py::array_t<typename Sequence::value_type> as_pyarray(Sequence &&seq, size_t col)
{
    auto size = seq.size();
    auto data = seq.data();
    std::unique_ptr<Sequence> seq_ptr = std::make_unique<Sequence>(std::move(seq));
    auto capsule = py::capsule(seq_ptr.get(),
                               [](void *p)
                               { std::unique_ptr<Sequence>(reinterpret_cast<Sequence *>(p)); });
    seq_ptr.release();
    return py::array({size/col, col}, data, capsule);
}

using dict = std::map<std::string, py::array>;
std::map<std::string, dict> vector_as_array_nocopy(std::size_t row, std::size_t col)
{

    dict map1;
    map1.insert({"hello_1", as_pyarray(std::move(make_vector<short>(row*col)), col)});
    map1.insert({"world_1", as_pyarray(std::move(make_vector<short>(row*col)), col)});

    dict map2;
    map2.insert({"hello_2", as_pyarray(std::move(make_vector<short>(row*col)), col)});

    std::vector<std::string> v;
    v.push_back("one\0");
    v.push_back("two\0");
    v.push_back("three\0");
    char* data = new char[6 * 3];
    for (int i = 0; i < v.size(); ++i)
    {
        std::strncpy(data+6*i, v[i].data(), 6);
    }


    map2.insert({"strings", py::array(py::dtype("S" + std::to_string(6)), {3}, {6}, data)});
    
    std::map<std::string, dict> res;

    res.insert({"map1", map1});
    res.insert({"map2", map2});

    return res;
}

PYBIND11_MODULE(fasttlogparser, m)
{
    m.def("return_numpy_array", &vector_as_array_nocopy);
#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}