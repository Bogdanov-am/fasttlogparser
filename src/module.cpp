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
    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size))
    {
        return buffer;
    } else {
        throw std::runtime_error("Error while file read!");
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
                                      std::optional<std::vector<MavId>> ids,
                                      std::optional<std::vector<std::string>> whitelist,
                                      std::optional<std::vector<std::string>> blacklist)
{
    auto data = readFile(path);
    std::map<std::string, std::shared_ptr<MessageSeries>> series_map;

    std::vector<MavId> ids_v = ids.has_value() ? ids.value() : std::vector<MavId>();
    std::vector<std::string> whitelist_v = whitelist.has_value() ? whitelist.value() : std::vector<std::string>();
    std::vector<std::string> blacklist_v = blacklist.has_value() ? blacklist.value() : std::vector<std::string>();
    std::set<std::string> whitelist_set(whitelist_v.begin(), whitelist_v.end());
    std::set<std::string> blacklist_set(blacklist_v.begin(), blacklist_v.end());

    mavlink_status_t status;
    mavlink_message_t msg;
    int chan = MAVLINK_COMM_0;

    for (size_t i = sizeof(uint64_t); i < data.size(); ++i)
    {
        uint8_t byte = data[i];
        if (mavlink_parse_char(chan, byte, &msg, &status))
        {
            i += sizeof(uint64_t);
            if (filterMsgById(ids_v, &msg))
            {
                continue;
            }
            if (filterMsgByBlackList(whitelist_set, blacklist_set, &msg))
            {
                continue;
            }

            const mavlink_message_info_t *msg_info = mavlink_get_message_info(&msg);
            if (!msg_info) {
                continue;
            }
            std::string msg_name(msg_info->name);
            size_t msg_offset = i - (12 + msg.len) - sizeof(uint64_t) + 1;
            auto pair = series_map.find(msg_name);
            if (pair != series_map.end())
            {
                pair->second->addOffsets(msg_offset);
            }
            else
            {
                auto series = std::make_shared<MessageSeries>(msg_info, data.data());
                series->addOffsets(msg_offset);
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

using namespace pybind11::literals;

PYBIND11_MODULE(fasttlogparser, m)
{
    m.def("parseTLog", &parseTLog, "path"_a, "ids"_a = py::none(), "whitelist"_a = py::none(), "blacklist"_a=py::none());
#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}