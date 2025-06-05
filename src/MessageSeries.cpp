#include "MessageSeries.h"

template <typename Sequence>
inline py::array_t<typename Sequence::value_type> as_pyarray(Sequence &&seq, size_t column)
{
    auto size = seq.size();
    auto data = seq.data();
    std::unique_ptr<Sequence> seq_ptr =
        std::make_unique<Sequence>(std::move(seq));
    auto capsule = py::capsule(seq_ptr.get(), [](void *p)
                               { std::unique_ptr<Sequence>(reinterpret_cast<Sequence *>(p)); });
    seq_ptr.release();

    if (column == 1)
    {
        return py::array(size, data, capsule);
    }
    else
    {
        return py::array({size / column, column}, data, capsule);
    }
}

template <typename Sequence>
inline py::array_t<typename Sequence::value_type> as_pyarray(Sequence &&seq)
{
    return as_pyarray(std::move(seq), 1);
}

MessageSeries::MessageSeries(const mavlink_message_info_t *info) : _info(info)
{
}

void MessageSeries::addMsg(uint64_t timestamp, const mavlink_message_t* msg)
{
    _timestamps.push_back(double(timestamp) / 1e6);
    mavlink_message_t cp_msg;
    memcpy(&cp_msg, msg, sizeof(mavlink_message_t));
    _msgs.push_back(cp_msg);
}

template <class T>
py::array_t<T> MessageSeries::getField(const mavlink_field_info_t *field_info)
{
    unsigned int column = field_info->array_length ? field_info->array_length : 1;
    std::vector<T> v(length() * column);
    T *dest = v.data();
    for (int i = 0; i < length(); ++i)
    {
        auto payload = _MAV_PAYLOAD(getMsg(i));
        memcpy(&dest[i * column], &payload[field_info->wire_offset], column * sizeof(T));
    }

    return as_pyarray(std::move(v), column);
}

py::array MessageSeries::getFieldChar(const mavlink_field_info_t *field_info)
{
    int column = field_info->array_length ? field_info->array_length : 1;
    column += 1;
    char *data = new char[column * length()];
    for (int i = 0; i < length(); ++i)
    {
        auto payload = _MAV_PAYLOAD(getMsg(i));
        std::strncpy(data + column * i, &payload[field_info->wire_offset], column);
    }
    return py::array(py::dtype("S" + std::to_string(column)), {length()}, {column}, data);
}

py::array MessageSeries::getField(const mavlink_field_info_t *field_info)
{
    switch (field_info->type)
    {
    case MAVLINK_TYPE_CHAR:
        return getFieldChar(field_info);
    case MAVLINK_TYPE_UINT8_T:
        return getField<uint8_t>(field_info);
    case MAVLINK_TYPE_INT8_T:
        return getField<int8_t>(field_info);
    case MAVLINK_TYPE_UINT16_T:
        return getField<uint16_t>(field_info);
    case MAVLINK_TYPE_INT16_T:
        return getField<int16_t>(field_info);
    case MAVLINK_TYPE_UINT32_T:
        return getField<uint32_t>(field_info);
    case MAVLINK_TYPE_INT32_T:
        return getField<int32_t>(field_info);
    case MAVLINK_TYPE_UINT64_T:
        return getField<uint64_t>(field_info);
    case MAVLINK_TYPE_INT64_T:
        return getField<int64_t>(field_info);
    case MAVLINK_TYPE_FLOAT:
        return getField<float>(field_info);
    case MAVLINK_TYPE_DOUBLE:
        return getField<double>(field_info);
    }
    
    throw std::logic_error("Unknown type!"); 
}

py::array MessageSeries::getTimestamps()
{
    return as_pyarray(std::move(_timestamps));
}

py::array MessageSeries::getSysIds()
{
    std::vector<uint8_t> v(length());
    for (int i = 0; i < length(); ++i)
    {
        v[i] = getMsg(i)->sysid;
    }
    return as_pyarray(std::move(v));
}

py::array MessageSeries::getCompIds()
{
    std::vector<uint8_t> v(length());
    for (int i = 0; i < length(); ++i)
    {
        v[i] = getMsg(i)->compid;
    }
    return as_pyarray(std::move(v));
}

mavlink_message_t *MessageSeries::getMsg(uint64_t index)
{
    return &_msgs[index];
}

std::string get_remap_name(const std::map<std::string, std::string> &remap_field, std::string field_name)
{
    auto pair = remap_field.find(field_name);
    if (pair != remap_field.end()) {
        return pair->second;
    } else {
        return field_name;
    }
}

std::map<std::string, py::array> MessageSeries::getFields(const std::map<std::string, std::string> &remap_field)
{
    std::map<std::string, py::array> map;
    map.insert({"timestamp", getTimestamps()});
    map.insert({"sys_id", getSysIds()});
    map.insert({"cmp_id", getCompIds()});

    for (size_t i = 0; i < _info->num_fields; ++i)
    {
        const mavlink_field_info_t field_info = _info->fields[i];
        std::string field_name = std::string(field_info.name);
        field_name = get_remap_name(remap_field, field_name);
        map.insert({field_name, getField(&field_info)});
    }
    return map;
}


