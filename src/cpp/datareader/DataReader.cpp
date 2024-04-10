// Copyright 2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <uxr/agent/datareader/DataReader.hpp>
#include <uxr/agent/participant/Participant.hpp>
#include <uxr/agent/topic/Topic.hpp>
#include <uxr/agent/client/ProxyClient.hpp>
#include <uxr/agent/utils/TokenBucket.hpp>
#include <uxr/agent/logger/Logger.hpp>

namespace eprosima {
namespace uxr {

#if defined(UAGENT_RESTRICT) || defined(UAGENT_PROTECT)
std::vector<std::string> DataReader::topic_frequency_array;
std::vector<DataReader::TopicInfo> DataReader::topic_info_;
int DataReader::topic_count;
std::vector<float> DataReader::frequency;
#endif

std::unique_ptr<DataReader> DataReader::create(
        const dds::xrce::ObjectId& object_id,
        uint16_t subscriber_id,
        const std::shared_ptr<ProxyClient>& proxy_client,
        const dds::xrce::DATAREADER_Representation& representation)
{
    bool created_entity = false;
    uint16_t raw_object_id = conversion::objectid_to_raw(object_id);
#if defined(UAGENT_RESTRICT) || defined(UAGENT_PROTECT)
    std::string topic_name = "";

    for (const auto &topic : topic_info_)
    {
        if (topic.objectID == raw_object_id)
        {
            topic_name = topic.TopicName;
        }
    }

    std::vector<float> matched_next_values;

    for (size_t i = 0; i < topic_frequency_array.size(); ++i)
    {
        if (topic_frequency_array[i].compare(topic_name) == 0 && i + 1 < topic_frequency_array.size())
        {
            for (auto &topic : topic_info_)
            {
                if (topic.objectID == raw_object_id)
                {
                    topic.frequency = std::stof(topic_frequency_array[i + 1]);
                    break;
                }
            }
        }
    }
#endif
    Middleware& middleware = proxy_client->get_middleware();
    switch (representation.representation()._d())
    {
        case dds::xrce::REPRESENTATION_BY_REFERENCE:
        {
            const std::string& ref = representation.representation().object_reference();
            created_entity =
                middleware.create_datareader_by_ref(raw_object_id, subscriber_id, ref);
            break;
        }
        case dds::xrce::REPRESENTATION_AS_XML_STRING:
        {
            const std::string& xml = representation.representation().xml_string_representation();
            created_entity =
                middleware.create_datareader_by_xml(raw_object_id, subscriber_id, xml);
            break;
        }
        case dds::xrce::REPRESENTATION_IN_BINARY:
        {
            auto rep = representation.representation();
            dds::xrce::OBJK_DataReader_Binary datareader_xrce;

            fastcdr::FastBuffer fastbuffer{reinterpret_cast<char*>(const_cast<uint8_t*>(rep.binary_representation().data())), rep.binary_representation().size()};
            eprosima::fastcdr::Cdr::Endianness endianness = static_cast<eprosima::fastcdr::Cdr::Endianness>(representation.endianness());
            eprosima::fastcdr::Cdr cdr(fastbuffer, endianness, eprosima::fastcdr::CdrVersion::XCDRv1);
            datareader_xrce.deserialize(cdr);

            created_entity = proxy_client->get_middleware().create_datareader_by_bin(raw_object_id, subscriber_id, datareader_xrce);
            break;
        }
        default:
            break;
    }

    return (created_entity ? std::unique_ptr<DataReader>(new DataReader(object_id, proxy_client)) : nullptr);
}

DataReader::DataReader(
        const dds::xrce::ObjectId& object_id,
        const std::shared_ptr<ProxyClient>& proxy_client)
    : XRCEObject{object_id}
    , proxy_client_{proxy_client}
    , reader_{}
{}

DataReader::~DataReader() noexcept
{
    reader_.stop_reading();
    proxy_client_->get_middleware().delete_datareader(get_raw_id());
}

bool DataReader::matched(
        const dds::xrce::ObjectVariant& new_object_rep) const
{
    /* Check ObjectKind. */
    if ((get_id().at(1) & 0x0F) != new_object_rep._d())
    {
        return false;
    }

    bool rv = false;
    switch (new_object_rep.data_reader().representation()._d())
    {
        case dds::xrce::REPRESENTATION_BY_REFERENCE:
        {
            const std::string& ref = new_object_rep.data_reader().representation().object_reference();
            rv = proxy_client_->get_middleware().matched_datareader_from_ref(get_raw_id(), ref);
            break;
        }
        case dds::xrce::REPRESENTATION_AS_XML_STRING:
        {
            const std::string& xml = new_object_rep.data_reader().representation().xml_string_representation();
            rv = proxy_client_->get_middleware().matched_datareader_from_xml(get_raw_id(), xml);
            break;
        }
        case dds::xrce::REPRESENTATION_IN_BINARY:
        {
            auto rep = new_object_rep.data_reader().representation();
            dds::xrce::OBJK_DataReader_Binary datareader_xrce;

            fastcdr::FastBuffer fastbuffer{reinterpret_cast<char*>(const_cast<uint8_t*>(rep.binary_representation().data())), rep.binary_representation().size()};
            eprosima::fastcdr::Cdr::Endianness endianness = static_cast<eprosima::fastcdr::Cdr::Endianness>(new_object_rep.endianness());
            eprosima::fastcdr::Cdr cdr(fastbuffer, endianness, eprosima::fastcdr::CdrVersion::XCDRv1);
            datareader_xrce.deserialize(cdr);

            rv = proxy_client_->get_middleware().matched_datareader_from_bin(get_raw_id(), datareader_xrce);
            break;
        }
        default:
            break;
    }
    return rv;
}

bool DataReader::read(
        const dds::xrce::READ_DATA_Payload& read_data,
        Reader<bool>::WriteFn write_fn,
        WriteFnArgs& write_args)
{
    dds::xrce::DataDeliveryControl delivery_control;
    if (read_data.read_specification().has_delivery_control())
    {
        delivery_control = read_data.read_specification().delivery_control();
    }
    else
    {
        delivery_control.max_elapsed_time(0);
        delivery_control.max_bytes_per_second(0);
        delivery_control.max_samples(1);
    }

    /* TODO (julianbermudez): implement different data formats.
    switch (read_data.read_specification().data_format())
    {
        case dds::xrce::FORMAT_DATA:
            break;
        case dds::xrce::FORMAT_SAMPLE:
            break;
        case dds::xrce::FORMAT_DATA_SEQ:
            break;
        case dds::xrce::FORMAT_SAMPLE_SEQ:
            break;
        case dds::xrce::FORMAT_PACKED_SAMPLES:
            break;
        default:
            break;
    }
    */

    write_args.client = proxy_client_;

    using namespace std::placeholders;
    return (reader_.stop_reading() &&
            reader_.start_reading(delivery_control, std::bind(&DataReader::read_fn, this, _1, _2, _3), false, write_fn, write_args));
}

bool DataReader::read_fn(
        bool,
        std::vector<uint8_t>& data,
        std::chrono::milliseconds timeout)
{
    bool rv = false;
    if (proxy_client_->get_middleware().read_data(get_raw_id(), data, timeout))
    {
#if defined(UAGENT_RESTRICT)
        for (auto &topic : topic_info_)
        {
            if (topic.objectID == get_raw_id())
            {
                if (topic.frequency == 1000){
                    UXR_AGENT_LOG_MESSAGE(
                        UXR_DECORATE_YELLOW("[==>> DDS <<==]"),
                        get_raw_id(),
                        data.data(),
                        data.size());
                    rv = true;
                }
            }
            else
            {
                if (topic.count < topic.frequency)
                {
        UXR_AGENT_LOG_MESSAGE(
            UXR_DECORATE_YELLOW("[==>> DDS <<==]"),
            get_raw_id(),
            data.data(),
            data.size());
        rv = true;
    }
                else
                {
                    topic.count = 0;
                    break;
                }
            }
        }
#elif defined(UAGENT_PROTECT)
        std::chrono::duration<double> diff = std::chrono::system_clock::now() - read_times_[get_raw_id()];
        float frequency_ = 0.001;
        for (const auto &topic : topic_info_)
        {
            if (topic.objectID == get_raw_id())
            {
                frequency_ = topic.frequency;
                break;
            }
        }

        if (diff.count() > frequency_)
        {
            read_times_[get_raw_id()] = std::chrono::system_clock::now();

            UXR_AGENT_LOG_MESSAGE(
                UXR_DECORATE_YELLOW("[==>> DDS <<==]"),
                get_raw_id(),
                data.data(),
                data.size());
            rv = true;
        }
#else
        UXR_AGENT_LOG_MESSAGE(
            UXR_DECORATE_YELLOW("[==>> DDS <<==]"),
            get_raw_id(),
            data.data(),
            data.size());
        rv = true;
#endif
    }
    return rv;
}

#if defined(UAGENT_PROTECT)
std::chrono::system_clock::time_point DataReader::get_read_time() const
{
    auto it = read_times_.find(get_raw_id());
    if (it != read_times_.end())
    {
        return it->second;
    }
    else
    {
        return std::chrono::system_clock::time_point();
    }
}
#endif

} // namespace uxr
} // namespace eprosima
