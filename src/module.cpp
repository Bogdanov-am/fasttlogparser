#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <iostream>
#include <fstream>

#define MAVLINK_USE_MESSAGE_INFO
#include <ardupilotmega/mavlink.h>
#include <mavlink_helpers.h>

#include "MessageSeries.h"

#include <cstdint>

uint64_t swapThis( uint64_t value )
{
#if defined(_MSC_VER)
    return _byteswap_uint64( value );
#else
    return __builtin_bswap64( value );
#endif
}

namespace py = pybind11;

std::vector<char> readFile(const std::string &path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    if (size == -1)
    {
        throw std::runtime_error("File not found! " + path);
    }
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size))
    {
        return buffer;
    }
    else
    {
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

using MavIds = std::map<uint8_t, std::set<uint8_t>>;
void addMavId(MavIds& ids, mavlink_message_t *msg)
{
    auto pair = ids.find(msg->sysid);
    if (pair != ids.end()) {
        pair->second.insert(msg->compid);
    } else {
        ids.insert({msg->sysid, std::set<uint8_t>({msg->compid})});
    }
}

using FieldsMap = std::map<std::string, py::array>;
using MessagesMap = std::map<std::string, FieldsMap>;

std::pair<MessagesMap, MavIds> parseTLog(const std::string &path,
                                      std::optional<std::vector<MavId>> ids_opt,
                                      std::optional<std::vector<std::string>> whitelist_opt,
                                      std::optional<std::vector<std::string>> blacklist_opt,
                                      std::optional<std::map<std::string, std::string>> remap_field_opt)
{
    std::vector<MavId> ids = ids_opt.has_value() ? ids_opt.value() : std::vector<MavId>();
    std::vector<std::string> whitelist = whitelist_opt.has_value() ? whitelist_opt.value() : std::vector<std::string>();
    std::vector<std::string> blacklist = blacklist_opt.has_value() ? blacklist_opt.value() : std::vector<std::string>();
    std::map<std::string, std::string> remap_field = remap_field_opt.has_value() ? remap_field_opt.value() : std::map<std::string, std::string>();
    
    std::set<std::string> whitelist_set(whitelist.begin(), whitelist.end());
    std::set<std::string> blacklist_set(blacklist.begin(), blacklist.end());

    auto data = readFile(path);
    std::map<std::string, std::shared_ptr<MessageSeries>> series_map;
    MavIds found_ids;
    
    mavlink_status_t status;
    mavlink_message_t msg;
    int chan = MAVLINK_COMM_0;

    uint64_t t = swapThis(*(uint64_t *)data.data());
    for (size_t i = sizeof(uint64_t); i < data.size(); ++i)
    {
        uint8_t byte = data[i];
        if (mavlink_parse_char(chan, byte, &msg, &status))
        {
            i += sizeof(uint64_t);
            addMavId(found_ids, &msg);
            if (filterMsgById(ids, &msg))
            {
                continue;
            }
            if (filterMsgByBlackList(whitelist_set, blacklist_set, &msg))
            {
                continue;
            }

            const mavlink_message_info_t *msg_info = mavlink_get_message_info(&msg);
            if (!msg_info)
            {
                continue;
            }
            
            std::string msg_name(msg_info->name);
            auto pair = series_map.find(msg_name);
            if (pair != series_map.end())
            {
                
                pair->second->addMsg(t, &msg);
            }
            else
            {
                auto series = std::make_shared<MessageSeries>(msg_info);
                series->addMsg(t, &msg);
                series_map.insert({msg_name, series});
            }
            t = swapThis(*(uint64_t *)(data.data() + i - sizeof(uint64_t) + 1));
        }
    }

    MessagesMap msg_map;
    for (auto pair : series_map)
    {
        msg_map.insert({pair.first, pair.second->getFields(remap_field)});
    }
    return std::pair{msg_map, found_ids};
}

using namespace pybind11::literals;

PYBIND11_MODULE(fasttlogparser, m)
{
    m.def("parseTLog", &parseTLog, 
        "path"_a, 
        "ids"_a = py::none(), 
        "whitelist"_a = py::none(), 
        "blacklist"_a = py::none(),
        "remap_field"_a = py::none());
#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}