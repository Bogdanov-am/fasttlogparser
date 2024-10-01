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

MessageSeries::MessageSeries(const mavlink_message_info_t *info, const char *data) : info(info), data(data)
{
}

MessageSeries::~MessageSeries()
{
}

void MessageSeries::addOffsets(uint64_t offset)
{
    msg_offsets.push_back(offset);
}

template <class T>
py::array_t<T> MessageSeries::getField(const mavlink_field_info_t *field_info)
{
    unsigned int column = field_info->array_length ? field_info->array_length : 1;
    std::vector<T> v(length() * column);

    for (int i = 0; i < length(); ++i)
    {
        size_t offset = msg_offsets[i];
        auto msg = getMsgByOffset(offset);
        auto payload = (char *)msg->payload64;

        for (size_t j = 0; j < column; ++j)
        {
            T data = *((T *)(payload + field_info->wire_offset) + j);
            v[i * column + j] = data;
        }
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
        size_t offset = msg_offsets[i];
        auto msg = getMsgByOffset(offset);
        auto payload = (char *)msg->payload64;

        char *str = payload + field_info->wire_offset;
        std::strncpy(data + column * i, str, column);
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
    std::vector<uint64_t> v(length());
    for (int i = 0; i < length(); ++i)
    {
        size_t offset = msg_offsets[i];
        v[i] = _byteswap_uint64(*(uint64_t *)(data + offset));
    }
    return as_pyarray(std::move(v));
}

py::array MessageSeries::getSysIds()
{
    std::vector<uint8_t> v(length());
    for (int i = 0; i < length(); ++i)
    {
        size_t offset = msg_offsets[i];
        v[i] = getMsgByOffset(offset)->sysid;
    }
    return as_pyarray(std::move(v));
}

py::array MessageSeries::getCompIds()
{
    std::vector<uint8_t> v(length());
    for (int i = 0; i < length(); ++i)
    {
        size_t offset = msg_offsets[i];
        v[i] = getMsgByOffset(offset)->compid;
    }
    return as_pyarray(std::move(v));
}

mavlink_message_t *MessageSeries::getMsgByOffset(uint64_t offset)
{
    return (mavlink_message_t *)(data + offset + sizeof(uint64_t) - sizeof(uint16_t) - 1);
}

std::map<std::string, py::array> MessageSeries::getFields()
{
    std::map<std::string, py::array> map;
    map.insert({"timestamp", getTimestamps()});
    map.insert({"sys_id", getSysIds()});
    map.insert({"cmp_id", getCompIds()});

    // py::print(std::string(info->name), length());
    for (size_t i = 0; i < info->num_fields; ++i)
    {
        const mavlink_field_info_t field_info = info->fields[i];
        // py::print(std::string(field_info.name), py::arg("end") = " ");
        map.insert({std::string(field_info.name), getField(&field_info)});
    }
    // py::print();

    return map;
}