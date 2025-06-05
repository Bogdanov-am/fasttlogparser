#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#define MAVLINK_USE_MESSAGE_INFO
#include <ardupilotmega/mavlink.h>
#include <mavlink_helpers.h>

namespace py = pybind11;

class MessageSeries
{
public:
    using Shr = std::shared_ptr<MessageSeries>;

    MessageSeries(const mavlink_message_info_t *info);

    void addMsg(uint64_t timestamp, const mavlink_message_t* msg);
    std::map<std::string, py::array> getFields(const std::map<std::string, std::string> &remap_field);
    
private:
    py::array getField(const mavlink_field_info_t *field_info);
    py::array getFieldChar(const mavlink_field_info_t *field_info);

    template <class T>
    py::array_t<T> getField(const mavlink_field_info_t *field_info);
    mavlink_message_t *getMsg(uint64_t index);
    py::array getTimestamps();
    py::array getSysIds();
    py::array getCompIds();
    size_t length() { return _msgs.size(); }

    const mavlink_message_info_t *_info;

    std::vector<double> _timestamps;
    std::vector<mavlink_message_t> _msgs;
};