#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <iostream>
#include <fstream>

#define MAVLINK_USE_MESSAGE_INFO
#include <ardupilotmega/mavlink.h>
#include <mavlink_helpers.h>

#include "MessageSeries.h"

namespace py = pybind11;

std::vector<char> readFile(const std::string &path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    if (size == -1) {
        throw std::runtime_error("File not found!");
    }
    file.seekg(0, std::ios::beg);
    py::print("Read file!", size);
    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size))
    {
        return buffer;
    } else {
        throw std::runtime_error("");
    }
}

using MavId = std::pair<uint8_t, uint8_t>;
bool filterMsgById(std::vector<MavId> ids, mavlink_message_t *msg)
{
    if (ids.size() == 0)
    {
        return false;
    }

    for (auto id : ids)
    {
        if (id.first == msg->sysid && id.second == msg->compid)
        {
            return false;
        }
    }

    return true;
}

bool filterMsgByBlackList(std::set<std::string> whitelist, std::set<std::string> backlist, mavlink_message_t *msg)
{
    const mavlink_message_info_t *msg_info = mavlink_get_message_info(msg);
    std::string msg_name(msg_info->name);

    if (whitelist.size() != 0)
    {
        if (whitelist.find(msg_name) == whitelist.end())
        {
            return true;
        }
    }

    if (backlist.find(msg_name) != backlist.end())
    {
        return true;
    }

    return false;
}

using dict = std::map<std::string, py::array>;
std::map<std::string, dict> parseTLog(const std::string &path,
                                      std::vector<MavId> ids,
                                      std::set<std::string> whitelist,
                                      std::set<std::string> backlist)
{
    auto data = readFile(path);
    std::map<std::string, std::shared_ptr<MessageSeries>> series_map;

    mavlink_status_t status;
    mavlink_message_t msg;
    int chan = MAVLINK_COMM_0;

    for (size_t i = sizeof(uint64_t); i < data.size(); ++i)
    {
        uint8_t byte = data[i];
        if (mavlink_parse_char(chan, byte, &msg, &status))
        {
            if (filterMsgById(ids, &msg))
            {
                continue;
            }
            if (filterMsgByBlackList(whitelist, backlist, &msg))
            {
                continue;
            }

            const mavlink_message_info_t *msg_info = mavlink_get_message_info(&msg);
            std::string msg_name(msg_info->name);
            auto pair = series_map.find(msg_name);
            if (pair != series_map.end())
            {
                pair->second->addOffsets(i - sizeof(uint64_t));
            }
            else
            {
                auto series = std::make_shared<MessageSeries>(msg_info, data.data());
                series->addOffsets(i - sizeof(uint64_t));
                series_map.insert({msg_name, series});
            }
        }
    }

    std::map<std::string, dict> result;
    for (auto pair : series_map)
    {
        result.insert({pair.first, pair.second->getFields()});
    }
    return result;
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
    return py::array({size / col, col}, data, capsule);
}

std::map<std::string, dict> vector_as_array_nocopy(std::size_t row, std::size_t col)
{

    dict map1;
    map1.insert({"hello_1", as_pyarray(std::move(make_vector<short>(row * col)), col)});
    map1.insert({"world_1", as_pyarray(std::move(make_vector<short>(row * col)), col)});

    dict map2;
    map2.insert({"hello_2", as_pyarray(std::move(make_vector<short>(row * col)), col)});

    std::vector<std::string> v;
    v.push_back("one\0");
    v.push_back("two\0");
    v.push_back("three\0");
    char *data = new char[6 * 3];
    for (int i = 0; i < v.size(); ++i)
    {
        std::strncpy(data + 6 * i, v[i].data(), 6);
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
    m.def("parseTLog", &parseTLog);
#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}