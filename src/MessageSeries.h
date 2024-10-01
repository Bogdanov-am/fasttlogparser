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

    MessageSeries(const mavlink_message_info_t *info, const char *data);
    ~MessageSeries();

    void addOffsets(uint64_t offset);
    std::map<std::string, py::array> getFields();
    
private:
    py::array getField(const mavlink_field_info_t *field_info);
    py::array getFieldChar(const mavlink_field_info_t *field_info);

    template <class T>
    py::array_t<T> getField(const mavlink_field_info_t *field_info);

    py::array getTimestamps();
    py::array getSysIds();
    py::array getCompIds();

    mavlink_message_t *getMsgByOffset(uint64_t offset);

    size_t length() { return msg_offsets.size(); }

    const char *data;
    const mavlink_message_info_t *info;
    std::vector<size_t> msg_offsets;
};