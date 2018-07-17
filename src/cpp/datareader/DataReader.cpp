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

#include <micrortps/agent/datareader/DataReader.hpp>
#include <micrortps/agent/utils/TokenBucket.hpp>
#include <fastrtps/Domain.h>
#include <xmlobjects/xmlobjects.h>

#include <fastrtps/subscriber/SampleInfo.h>
#include <fastrtps/subscriber/Subscriber.h>
#include <fastrtps/xmlparser/XMLProfileManager.h>

#include <atomic>

#define DEFAULT_XRCE_PARTICIPANT_PROFILE "default_xrce_participant_profile"
#define DEFAULT_XRCE_SUBSCRIBER_PROFILE "default_xrce_subscriber_profile"

namespace eprosima {
namespace micrortps {

using utils::TokenBucket;

DataReader::DataReader(const dds::xrce::ObjectId& id,
                       eprosima::fastrtps::Participant& rtps_participant,
                       const std::string& profile_name)
    : XRCEObject(id),
      running_cond_(false),
      rtps_subscriber_prof_(profile_name),
      rtps_participant_(rtps_participant),
      rtps_subscriber_(nullptr),
      topic_type_(false)
{
}

DataReader::~DataReader() noexcept
{
    stop_read();
    if (read_thread_.joinable())
    {
        read_thread_.join();
    }

    if (max_timer_thread_.joinable())
    {
        max_timer_thread_.join();
    }

    if (nullptr != rtps_subscriber_)
    {
        fastrtps::Domain::removeSubscriber(rtps_subscriber_);
    }
}

bool DataReader::init()
{
    SubscriberAttributes attributes;
    if (rtps_subscriber_prof_.empty() ||
        (fastrtps::xmlparser::XMLP_ret::XML_ERROR ==
         fastrtps::xmlparser::XMLProfileManager::fillSubscriberAttributes(rtps_subscriber_prof_, attributes)))
    {
        fastrtps::xmlparser::XMLProfileManager::getDefaultSubscriberAttributes(attributes);
    }
    if (check_registered_topic(attributes.topic.getTopicDataType()))
    {

        if (!rtps_subscriber_prof_.empty())
        {
            rtps_subscriber_ = fastrtps::Domain::createSubscriber(&rtps_participant_, rtps_subscriber_prof_, this);
        }
        else
        {
            rtps_subscriber_ =
                fastrtps::Domain::createSubscriber(&rtps_participant_, DEFAULT_XRCE_SUBSCRIBER_PROFILE, this);
        }
    }
    if (rtps_subscriber_ == nullptr)
    {
        std::cout << "init subscriber error" << std::endl;
        return false;
    }
    return true;
}

bool DataReader::init(const std::string& xmlrep)
{
    SubscriberAttributes attributes;
    if (xmlobjects::parse_subscriber(xmlrep.data(), xmlrep.size(), attributes))
    {
        if (check_registered_topic(attributes.topic.getTopicDataType()))
        {
            rtps_subscriber_ = fastrtps::Domain::createSubscriber(&rtps_participant_, attributes, this);
        }
    }
    else
    {
        fastrtps::xmlparser::XMLProfileManager::getDefaultSubscriberAttributes(attributes);
        if (check_registered_topic(attributes.topic.getTopicDataType()))
        {
            rtps_subscriber_ =
                fastrtps::Domain::createSubscriber(&rtps_participant_, DEFAULT_XRCE_SUBSCRIBER_PROFILE, this);
        }
    }
    if (rtps_subscriber_ == nullptr)
    {
        std::cout << "init subscriber error" << std::endl;
        return false;
    }
    return true;
}

void DataReader::read(const dds::xrce::READ_DATA_Payload& read_data,
                      read_callback read_cb, const ReadCallbackArgs& cb_args)
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
            std::cout << "Error: read format unexpected" << std::endl;
            break;
    }

    stop_read();
    start_read(delivery_control, read_cb, cb_args);
}

bool DataReader::has_message() const
{
    return msg_;
}

int DataReader::start_read(const dds::xrce::DataDeliveryControl& delivery_control, read_callback read_cb, const ReadCallbackArgs& cb_args)
{
    std::unique_lock<std::mutex> lock(mtx_);
    running_cond_ = true;
    lock.unlock();

    if (delivery_control.max_elapsed_time() > 0)
    {
        max_timer_thread_ = std::thread(&DataReader::run_max_timer, this, delivery_control.max_elapsed_time());
    }
    read_thread_ = std::thread(&DataReader::read_task, this, delivery_control, read_cb, cb_args);

    return 0;
}

int DataReader::stop_read()
{
    std::unique_lock<std::mutex> lock(mtx_);
    running_cond_ = false;
    lock.unlock();
    cond_var_.notify_one();

    if (read_thread_.joinable())
    {
        read_thread_.join();
    }

    stop_max_timer();
    if (max_timer_thread_.joinable())
    {
        max_timer_thread_.join();
    }
    return 0;
}

void DataReader::read_task(dds::xrce::DataDeliveryControl delivery_control,
                           read_callback read_cb, ReadCallbackArgs cb_args)
{
    TokenBucket rate_manager{delivery_control.max_bytes_per_second()};
    uint16_t message_count = 0;
    std::chrono::steady_clock::time_point last_read = std::chrono::steady_clock::now();
    while (true)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (running_cond_ && (message_count < delivery_control.max_samples()))
        {
            if (rtps_subscriber_->getUnreadCount() != 0)
            {
                lock.unlock();
                /* Read operation. */
                size_t next_data_size = nextDataSize();
                if (next_data_size != 0u && rate_manager.get_tokens(next_data_size))
                {
                    std::vector<unsigned char> buffer;
                    if (takeNextData(&buffer))
                    {
                        last_read = std::chrono::steady_clock::now();
                        read_cb(cb_args, buffer);
                        ++message_count;
                    }
                }
            }
            else
            {
                /* Wait for new message or terminate signal. */
                cond_var_.wait(lock);
                lock.unlock();
            }
        }
        else
        {
            running_cond_ = false;
            lock.unlock();
            stop_max_timer();
            break;
        }
    }
}

void DataReader::on_max_timeout(const asio::error_code& error)
{
    if (!error)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        running_cond_ = false;
        cond_var_.notify_one();
    }
}

void DataReader::onNewDataMessage(eprosima::fastrtps::Subscriber* /*sub*/)
{
    std::lock_guard<std::mutex> lock(mtx_);
    cond_var_.notify_one();
}

ReadTimeEvent::ReadTimeEvent()
    : m_timer_max(m_io_service_max)
{
}

int ReadTimeEvent::init_max_timer(int milliseconds)
{
    m_io_service_max.reset();
    m_timer_max.expires_from_now(std::chrono::milliseconds(milliseconds));
    m_timer_max.async_wait(std::bind(&ReadTimeEvent::on_max_timeout, this, std::placeholders::_1));
    return 0;
}

void ReadTimeEvent::stop_max_timer()
{
    m_timer_max.cancel();
    m_io_service_max.stop();
}

void ReadTimeEvent::run_max_timer(int milliseconds)
{
    init_max_timer(milliseconds);
    m_io_service_max.run();
}

bool DataReader::takeNextData(void* data)
{
    if (nullptr == rtps_subscriber_)
    {
        return false;
    }
    fastrtps::SampleInfo_t info;
    return rtps_subscriber_->takeNextData(data, &info);
}

size_t DataReader::nextDataSize()
{
    std::vector<unsigned char> buffer;
    fastrtps::SampleInfo_t info;
    // TODO (julian): review KEE_PALL configuration.
    if (rtps_subscriber_->readNextData(&buffer, &info))
    {
        if (info_.sampleKind == rtps::ALIVE)
        {
            if (!msg_)
            {
                msg_ = true;
            }
            return buffer.size();
        }
    }
    return 0;
}

void DataReader::onSubscriptionMatched(fastrtps::Subscriber* /*sub*/, fastrtps::rtps::MatchingInfo& info)
{
    if (info.status == rtps::MATCHED_MATCHING)
    {
        matched_++;
        std::cout << "RTPS Publisher matched" << std::endl;
    }
    else
    {
        matched_--;
        std::cout << "RTPS Publisher unmatched" << std::endl;
    }
}

bool DataReader::check_registered_topic(const std::string& topic_data_type) const
{
    // TODO (Borja): take this method out to Topic type.
    TopicDataType* p_type = nullptr;
    if (!fastrtps::Domain::getRegisteredType(&rtps_participant_, topic_data_type.data(), &p_type))
    {
        std::cout << "DDS ERROR: No registered type" << std::endl;
        return false;
    }
    return true;
}

} // namespace micrortps
} // namespace eprosima
