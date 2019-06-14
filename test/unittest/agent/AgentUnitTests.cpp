// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <uxr/agent/Agent.hpp>

#include <gtest/gtest.h>

namespace eprosima {
namespace uxr {
namespace testing {

using namespace eprosima::uxr;

class AgentUnitTests : public ::testing::Test
{
protected:
    AgentUnitTests()
    {
        agent_.load_config_file("./agent.refs");
    }

    ~AgentUnitTests()
    {
        agent_.reset();
    }

    eprosima::uxr::Agent agent_;
    const uint32_t client_key_ = 0xAABBCCDD;
};

TEST_F(AgentUnitTests, CreateClient)
{
    Agent::OpResult result;
    EXPECT_TRUE(agent_.create_client(client_key_, 0x01, 512, Middleware::Kind::FAST, result));
    EXPECT_TRUE(agent_.create_client(client_key_, 0x01, 512, Middleware::Kind::FAST, result));
}

TEST_F(AgentUnitTests, DeleteClient)
{
    Agent::OpResult result;
    agent_.create_client(client_key_, 0x01, 512, Middleware::Kind::FAST, result);

    EXPECT_TRUE(agent_.delete_client(client_key_, result));
    EXPECT_FALSE(agent_.delete_client(client_key_, result));
    EXPECT_EQ(result, agent_.OpResult::UNKNOWN_REFERENCE_ERROR);
}

TEST_F(AgentUnitTests, CreateParticipantByRef)
{
    Agent::OpResult result;
    agent_.create_client(client_key_, 0x01, 512, Middleware::Kind::FAST, result);

    const char* ref_one = "default_xrce_participant";
    const char* ref_two = "default_xrce_participant_two";

    const uint16_t participant_id = 0x00;
    const int16_t domain_id = 0x00;
    uint8_t flag = 0x00;

    /*
     * Create Participant.
     */
    EXPECT_TRUE(agent_.create_participant_by_ref(client_key_, participant_id, domain_id, ref_one, flag, result));

    /*
     * Create Participant over an existing with 0x00 flag.
     */
    EXPECT_FALSE(agent_.create_participant_by_ref(client_key_, participant_id, domain_id, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);
    EXPECT_FALSE(agent_.create_participant_by_ref(client_key_, participant_id, domain_id, ref_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);

    /*
     * Create Participant over an existing with REUSE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE;
    EXPECT_TRUE(agent_.create_participant_by_ref(client_key_, participant_id, domain_id, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);
    EXPECT_FALSE(agent_.create_participant_by_ref(client_key_, participant_id, domain_id, ref_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::MISMATCH_ERROR);

    /*
     * Create Participant over an existing with REPLACE flag.
     */
    flag = agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_participant_by_ref(client_key_, participant_id, domain_id, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);
    EXPECT_TRUE(agent_.create_participant_by_ref(client_key_, participant_id, domain_id, ref_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Create Participant over an existing with REUSE & REPLACE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE | agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_participant_by_ref(client_key_, participant_id, domain_id, ref_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);
    EXPECT_TRUE(agent_.create_participant_by_ref(client_key_, participant_id, domain_id, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Delete Participant.
     */
    EXPECT_TRUE(agent_.delete_participant(client_key_, participant_id, result));

    /*
     * Create Participant with invalid REF.
     */
    EXPECT_FALSE(agent_.create_participant_by_ref(client_key_, participant_id, domain_id, "error", flag, result));
    // TODO (Julian): add when the reference data base is done.
    //EXPECT_EQ(errcode, agent_.ErrorCode::UNKNOWN_REFERENCE_ERRCODE);
}

TEST_F(AgentUnitTests, CreateParticipantByXml)
{
    Agent::OpResult result;
    agent_.create_client(client_key_, 0x01, 512, Middleware::Kind::FAST, result);

    const char* xml_one = "<dds>"
                              "<participant>"
                                  "<rtps>"
                                      "<name>default_xrce_participant_one</name>"
                                  "</rtps>"
                              "</participant>"
                          "</dds>";
    const char* xml_two = "<dds>"
                              "<participant>"
                                  "<rtps>"
                                      "<name>default_xrce_participant_two</name>"
                                  "</rtps>"
                              "</participant>"
                          "</dds>";

    const uint16_t participant_id = 0x00;
    const int16_t domain_id = 0x00;
    uint8_t flag = 0x00;

    /*
     * Create Participant.
     */
    EXPECT_TRUE(agent_.create_participant_by_xml(client_key_, participant_id, domain_id, xml_one, flag, result));

    /*
     * Create Participant over an existing with 0x00 flag.
     */
    EXPECT_FALSE(agent_.create_participant_by_xml(client_key_, participant_id, domain_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);
    EXPECT_FALSE(agent_.create_participant_by_xml(client_key_, participant_id, domain_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);

    /*
     * Create Participant over an existing with REUSE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE;
    EXPECT_TRUE(agent_.create_participant_by_xml(client_key_, participant_id, domain_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);
    EXPECT_FALSE(agent_.create_participant_by_xml(client_key_, participant_id, domain_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::MISMATCH_ERROR);

    /*
     * Create Participant over an existing with REPLACE flag.
     */
    flag = agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_participant_by_xml(client_key_, participant_id, domain_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);
    EXPECT_TRUE(agent_.create_participant_by_xml(client_key_, participant_id, domain_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Create Participant over an existing with REUSE & REPLACE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE | agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_participant_by_xml(client_key_, participant_id, domain_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);
    EXPECT_TRUE(agent_.create_participant_by_xml(client_key_, participant_id, domain_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Delete Participant.
     */
    EXPECT_TRUE(agent_.delete_participant(client_key_, participant_id, result));

    /*
     * Create Participant with invalid XML.
     */
    EXPECT_FALSE(agent_.create_participant_by_xml(client_key_, participant_id, domain_id, "error", flag, result));
    // TODO (Julian): add when the reference data base is done.
    //EXPECT_EQ(errcode, agent_.ErrorCode::UNKNOWN_REFERENCE_ERRCODE);
}

TEST_F(AgentUnitTests, CreateTopicByRef)
{
    Agent::OpResult result;
    agent_.create_client(client_key_, 0x01, 512, Middleware::Kind::FAST, result);

    const char* participant_ref = "default_xrce_participant";
    const char* ref_one = "shapetype_topic";
    const char* ref_two = "helloworld_topic";

    const uint16_t topic_id = 0x00;
    const uint16_t participant_id = 0x00;
    const int16_t domain_id = 0x00;
    uint8_t flag = 0x00;

    /*
     * Create Topic.
     */
    EXPECT_TRUE(agent_.create_participant_by_ref(client_key_, participant_id, domain_id, participant_ref, flag, result));
    EXPECT_TRUE(agent_.create_topic_by_ref(client_key_, topic_id, participant_id, ref_one, flag, result));

    /*
     * Create Topic over an existing with 0x00 flag.
     */
    EXPECT_FALSE(agent_.create_topic_by_ref(client_key_, topic_id, participant_id, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);
    EXPECT_FALSE(agent_.create_topic_by_ref(client_key_, topic_id, participant_id, ref_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);

    /*
     * Create Topic over an existing with REUSE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE;
    EXPECT_TRUE(agent_.create_topic_by_ref(client_key_, topic_id, participant_id, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);
    EXPECT_FALSE(agent_.create_topic_by_ref(client_key_, topic_id, participant_id, ref_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::MISMATCH_ERROR);

    /*
     * Create Topic over an existing with REPLACE flag.
     */
    flag = agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_topic_by_ref(client_key_, topic_id, participant_id, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);
    EXPECT_TRUE(agent_.create_topic_by_ref(client_key_, topic_id, participant_id, ref_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Create Topic over an existing with REUSE & REPLACE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE | agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_topic_by_ref(client_key_, topic_id, participant_id, ref_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);
    EXPECT_TRUE(agent_.create_topic_by_ref(client_key_, topic_id, participant_id, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Delete Topic.
     */
    EXPECT_TRUE(agent_.delete_topic(client_key_, topic_id, result));

    /*
     * Create Topic with invalid REF.
     */
    EXPECT_FALSE(agent_.create_topic_by_ref(client_key_, topic_id, participant_id, "error", flag, result));
    // TODO (Julian): add when the reference data base is done.
    //EXPECT_EQ(errcode, agent_.ErrorCode::UNKNOWN_REFERENCE_ERRCODE);

    /*
     * Create Topic with unknown Participant.
     */
    EXPECT_FALSE(agent_.create_topic_by_ref(client_key_, topic_id, 0x01, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::UNKNOWN_REFERENCE_ERROR);
}

TEST_F(AgentUnitTests, CreateTopicByXml)
{
    Agent::OpResult result;
    agent_.create_client(client_key_, 0x01, 512, Middleware::Kind::FAST, result);

    const char* participant_ref = "default_xrce_participant";
    const char* xml_one = "<dds>"
                              "<topic>"
                                  "<kind>WITH_KEY</kind>"
                                  "<name>Square</name>"
                                  "<dataType>ShapeType</dataType>"
                              "</topic>"
                          "</dds>";
    const char* xml_two = "<dds>"
                              "<topic>"
                                  "<name>HelloWorldTopic</name>"
                                  "<dataType>HelloWorld</dataType>"
                              "</topic>"
                          "</dds>";

    const uint16_t topic_id = 0x00;
    const uint16_t participant_id = 0x00;
    const int16_t domain_id = 0x00;
    uint8_t flag = 0x00;

    /*
     * Create Topic.
     */
    EXPECT_TRUE(agent_.create_participant_by_ref(client_key_, participant_id, domain_id, participant_ref, flag, result));
    EXPECT_TRUE(agent_.create_topic_by_xml(client_key_, topic_id, participant_id, xml_one, flag, result));

    /*
     * Create Topic over an existing with 0x00 flag.
     */
    EXPECT_FALSE(agent_.create_topic_by_xml(client_key_, topic_id, participant_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);
    EXPECT_FALSE(agent_.create_topic_by_xml(client_key_, topic_id, participant_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);

    /*
     * Create Topic over an existing with REUSE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE;
    EXPECT_TRUE(agent_.create_topic_by_xml(client_key_, topic_id, participant_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);
    EXPECT_FALSE(agent_.create_topic_by_xml(client_key_, topic_id, participant_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::MISMATCH_ERROR);

    /*
     * Create Topic over an existing with REPLACE flag.
     */
    flag = agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_topic_by_xml(client_key_, topic_id, participant_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);
    EXPECT_TRUE(agent_.create_topic_by_xml(client_key_, topic_id, participant_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Create Topic over an existing with REUSE & REPLACE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE | agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_topic_by_xml(client_key_, topic_id, participant_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);
    EXPECT_TRUE(agent_.create_topic_by_xml(client_key_, topic_id, participant_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Delete Topic.
     */
    EXPECT_TRUE(agent_.delete_topic(client_key_, topic_id, result));

    /*
     * Create Topic with invalid XML.
     */
    EXPECT_FALSE(agent_.create_topic_by_xml(client_key_, topic_id, participant_id, "error", flag, result));
    // TODO (Julian): add when the reference data base is done.
    //EXPECT_EQ(errcode, agent_.ErrorCode::UNKNOWN_REFERENCE_ERRCODE);

    /*
     * Create Topic with unknown Participant.
     */
    EXPECT_FALSE(agent_.create_topic_by_xml(client_key_, topic_id, 0x01, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::UNKNOWN_REFERENCE_ERROR);
}

TEST_F(AgentUnitTests, CreatePublisherByXml)
{
    Agent::OpResult result;
    agent_.create_client(client_key_, 0x01, 512, Middleware::Kind::FAST, result);

    const char* participant_ref = "default_xrce_participant";
    const char* xml_one = "publisher_one";
    const char* xml_two = "publisher_two";

    const uint16_t publisher_id = 0x00;
    const uint16_t participant_id = 0x00;
    const int16_t domain_id = 0x00;
    uint8_t flag = 0x00;

    /*
     * Create Publisher.
     */
    EXPECT_TRUE(agent_.create_participant_by_ref(client_key_, participant_id, domain_id, participant_ref, flag, result));
    EXPECT_TRUE(agent_.create_publisher_by_xml(client_key_, publisher_id, participant_id, xml_one, flag, result));

    /*
     * Create Publisher over an existing with 0x00 flag.
     */
    EXPECT_FALSE(agent_.create_publisher_by_xml(client_key_, publisher_id, participant_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);
    EXPECT_FALSE(agent_.create_publisher_by_xml(client_key_, publisher_id, participant_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);

    /*
     * Create Publisher over an existing with REUSE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE;
    EXPECT_TRUE(agent_.create_publisher_by_xml(client_key_, publisher_id, participant_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);

    /*
     * Create Publisher over an existing with REPLACE flag.
     */
    flag = agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_publisher_by_xml(client_key_, publisher_id, participant_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);
    EXPECT_TRUE(agent_.create_publisher_by_xml(client_key_, publisher_id, participant_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Create Publisher over an existing with REUSE & REPLACE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE | agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_publisher_by_xml(client_key_, publisher_id, participant_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);

    /*
     * Delete Publisher.
     */
    EXPECT_TRUE(agent_.delete_publisher(client_key_, publisher_id, result));

    /*
     * Create Publisher with unknown Participant.
     */
    EXPECT_FALSE(agent_.create_publisher_by_xml(client_key_, publisher_id, 0x01, xml_one, flag, result));
    // TODO (Julian): add when the reference data base is done.
    //EXPECT_EQ(errcode, agent_.ErrorCode::UNKNOWN_REFERENCE_ERRCODE);
}

TEST_F(AgentUnitTests, CreateSubscriberByXml)
{
    Agent::OpResult result;
    agent_.create_client(client_key_, 0x01, 512, Middleware::Kind::FAST, result);

    const char* participant_ref = "default_xrce_participant";
    const char* xml_one = "subscriber_one";
    const char* xml_two = "subscriber_two";

    const uint16_t subscriber_id = 0x00;
    const uint16_t participant_id = 0x00;
    const int16_t domain_id = 0x00;
    uint8_t flag = 0x00;

    /*
     * Create Subscriber.
     */
    EXPECT_TRUE(agent_.create_participant_by_ref(client_key_, participant_id, domain_id, participant_ref, flag, result));
    EXPECT_TRUE(agent_.create_subscriber_by_xml(client_key_, subscriber_id, participant_id, xml_one, flag, result));

    /*
     * Create Subscriber over an existing with 0x00 flag.
     */
    EXPECT_FALSE(agent_.create_subscriber_by_xml(client_key_, subscriber_id, participant_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);
    EXPECT_FALSE(agent_.create_subscriber_by_xml(client_key_, subscriber_id, participant_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);

    /*
     * Create Subscriber over an existing with REUSE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE;
    EXPECT_TRUE(agent_.create_subscriber_by_xml(client_key_, subscriber_id, participant_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);

    /*
     * Create Subscriber over an existing with REPLACE flag.
     */
    flag = agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_subscriber_by_xml(client_key_, subscriber_id, participant_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);
    EXPECT_TRUE(agent_.create_subscriber_by_xml(client_key_, subscriber_id, participant_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Create Subscriber over an existing with REUSE & REPLACE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE | agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_subscriber_by_xml(client_key_, subscriber_id, participant_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);

    /*
     * Delete Subscriber.
     */
    EXPECT_TRUE(agent_.delete_subscriber(client_key_, subscriber_id, result));

    /*
     * Create Subscriber with unknown Participant.
     */
    EXPECT_FALSE(agent_.create_subscriber_by_xml(client_key_, subscriber_id, 0x01, xml_one, flag, result));
    // TODO (Julian): add when the reference data base is done.
    //EXPECT_EQ(errcode, agent_.ErrorCode::UNKNOWN_REFERENCE_ERRCODE);
}

TEST_F(AgentUnitTests, CreateDataWriterByRef)
{
    Agent::OpResult result;
    agent_.create_client(client_key_, 0x01, 512, Middleware::Kind::FAST, result);

    const char* participant_ref = "default_xrce_participant";
    const char* topic_ref = "shapetype_topic";
    const char* publisher_xml = "publisher";
    const char* ref_one = "shapetype_data_writer";
    const char* ref_two = "shapetype_data_writer_two";

    const int16_t domain_id = 0x00;
    const uint16_t participant_id = 0x00;
    const uint16_t topic_id = 0x00;
    const uint16_t publisher_id = 0x00;
    const uint16_t datawriter_id = 0x00;

    uint8_t flag = 0x00;

    /*
     * Create DataWriter.
     */
    agent_.create_participant_by_ref(client_key_, participant_id, domain_id, participant_ref, flag, result);
    agent_.create_topic_by_ref(client_key_, topic_id, participant_id, topic_ref, flag, result);
    agent_.create_publisher_by_xml(client_key_, publisher_id, participant_id, publisher_xml, flag, result);
    EXPECT_TRUE(agent_.create_datawriter_by_ref(client_key_, datawriter_id, publisher_id, ref_one, flag, result));

    /*
     * Create DataWriter over an existing with 0x00 flag.
     */
    EXPECT_FALSE(agent_.create_datawriter_by_ref(client_key_, datawriter_id, publisher_id, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);
    EXPECT_FALSE(agent_.create_datawriter_by_ref(client_key_, datawriter_id, publisher_id, ref_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);

    /*
     * Create DataWriter over an existing with REUSE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE;
    EXPECT_TRUE(agent_.create_datawriter_by_ref(client_key_, datawriter_id, publisher_id, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);
    EXPECT_FALSE(agent_.create_datawriter_by_ref(client_key_, datawriter_id, publisher_id, ref_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::MISMATCH_ERROR);

    /*
     * Create DataWriter over an existing with REPLACE flag.
     */
    flag = agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_datawriter_by_ref(client_key_, datawriter_id, publisher_id, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);
    EXPECT_TRUE(agent_.create_datawriter_by_ref(client_key_, datawriter_id, publisher_id, ref_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Create DataWriter over an existing with REUSE & REPLACE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE | agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_datawriter_by_ref(client_key_, datawriter_id, publisher_id, ref_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);
    EXPECT_TRUE(agent_.create_datawriter_by_ref(client_key_, datawriter_id, publisher_id, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Delete DataWriter.
     */
    EXPECT_TRUE(agent_.delete_datawriter(client_key_, datawriter_id, result));

    /*
     * Create DataWriter with invalid REF.
     */
    EXPECT_FALSE(agent_.create_datawriter_by_ref(client_key_, datawriter_id, publisher_id, "error", flag, result));
    // TODO (Julian): add when the reference data base is done.
    //EXPECT_EQ(errcode, agent_.ErrorCode::UNKNOWN_REFERENCE_ERRCODE);

    /*
     * Create DataWriter with unknown Publisher.
     */
    EXPECT_FALSE(agent_.create_datawriter_by_ref(client_key_, datawriter_id, 0x01, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::UNKNOWN_REFERENCE_ERROR);
}

TEST_F(AgentUnitTests, CreateDataWriterByXml)
{
    Agent::OpResult result;
    agent_.create_client(client_key_, 0x01, 512, Middleware::Kind::FAST, result);

    const char* participant_ref = "default_xrce_participant";
    const char* topic_ref = "helloworld_topic";
    const char* publisher_xml = "publisher";
    const char* xml_one = "<dds>"
                              "<data_writer>"
                                  "<topic>"
                                      "<kind>NO_KEY</kind>"
                                      "<name>HelloWorldTopic</name>"
                                      "<dataType>HelloWorld</dataType>"
                                      "<historyQos>"
                                          "<kind>KEEP_LAST</kind>"
                                          "<depth>5</depth>"
                                      "</historyQos>"
                                  "</topic>"
                                  "<qos>"
                                      "<durability>"
                                          "<kind>TRANSIENT_LOCAL</kind>"
                                      "</durability>"
                                  "</qos>"
                              "</data_writer>"
                          "</dds>";
    const char* xml_two = "<dds>"
                              "<data_writer>"
                                  "<topic>"
                                      "<kind>NO_KEY</kind>"
                                      "<name>HelloWorldTopic</name>"
                                      "<dataType>HelloWorld</dataType>"
                                      "<historyQos>"
                                          "<kind>KEEP_LAST</kind>"
                                          "<depth>10</depth>"
                                      "</historyQos>"
                                  "</topic>"
                                  "<qos>"
                                      "<durability>"
                                          "<kind>TRANSIENT_LOCAL</kind>"
                                      "</durability>"
                                  "</qos>"
                              "</data_writer>"
                          "</dds>";

    const int16_t domain_id = 0x00;
    const uint16_t participant_id = 0x00;
    const uint16_t topic_id = 0x00;
    const uint16_t publisher_id = 0x00;
    const uint16_t datawriter_id = 0x00;

    uint8_t flag = 0x00;

    /*
     * Create DataWriter.
     */
    agent_.create_participant_by_ref(client_key_, participant_id, domain_id, participant_ref, flag, result);
    agent_.create_topic_by_ref(client_key_, topic_id, participant_id, topic_ref, flag, result);
    agent_.create_publisher_by_xml(client_key_, publisher_id, participant_id, publisher_xml, flag, result);
    EXPECT_TRUE(agent_.create_datawriter_by_xml(client_key_, datawriter_id, publisher_id, xml_one, flag, result));

    /*
     * Create DataWriter over an existing with 0x00 flag.
     */
    EXPECT_FALSE(agent_.create_datawriter_by_xml(client_key_, datawriter_id, publisher_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);
    EXPECT_FALSE(agent_.create_datawriter_by_xml(client_key_, datawriter_id, publisher_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);

    /*
     * Create DataWriter over an existing with REUSE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE;
    EXPECT_TRUE(agent_.create_datawriter_by_xml(client_key_, datawriter_id, publisher_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);
    EXPECT_FALSE(agent_.create_datawriter_by_xml(client_key_, datawriter_id, publisher_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::MISMATCH_ERROR);

    /*
     * Create DataWriter over an existing with REPLACE flag.
     */
    flag = agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_datawriter_by_xml(client_key_, datawriter_id, publisher_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);
    EXPECT_TRUE(agent_.create_datawriter_by_xml(client_key_, datawriter_id, publisher_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Create DataWriter over an existing with REUSE & REPLACE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE | agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_datawriter_by_xml(client_key_, datawriter_id, publisher_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);
    EXPECT_TRUE(agent_.create_datawriter_by_xml(client_key_, datawriter_id, publisher_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Delete DataWriter.
     */
    EXPECT_TRUE(agent_.delete_datawriter(client_key_, datawriter_id, result));

    /*
     * Create DataWriter with invalid REF.
     */
    EXPECT_FALSE(agent_.create_datawriter_by_xml(client_key_, datawriter_id, publisher_id, "error", flag, result));
    // TODO (Julian): add when the reference data base is done.
    //EXPECT_EQ(errcode, agent_.ErrorCode::UNKNOWN_REFERENCE_ERRCODE);

    /*
     * Create DataWriter with unknown Publisher.
     */
    EXPECT_FALSE(agent_.create_datawriter_by_xml(client_key_, datawriter_id, 0x01, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::UNKNOWN_REFERENCE_ERROR);
}

TEST_F(AgentUnitTests, CreateDataReaderByRef)
{
    Agent::OpResult result;
    agent_.create_client(client_key_, 0x01, 512, Middleware::Kind::FAST, result);

    const char* participant_ref = "default_xrce_participant";
    const char* topic_ref = "shapetype_topic";
    const char* subscriber_xml = "subscriber";
    const char* ref_one = "shapetype_data_reader";
    const char* ref_two = "shapetype_data_reader_two";

    const int16_t domain_id = 0x00;
    const uint16_t participant_id = 0x00;
    const uint16_t topic_id = 0x00;
    const uint16_t subscriber_id = 0x00;
    const uint16_t datareader_id = 0x00;

    uint8_t flag = 0x00;

    /*
     * Create DataReader.
     */
    agent_.create_participant_by_ref(client_key_, participant_id, domain_id, participant_ref, flag, result);
    agent_.create_topic_by_ref(client_key_, topic_id, participant_id, topic_ref, flag, result);
    agent_.create_subscriber_by_xml(client_key_, subscriber_id, participant_id, subscriber_xml, flag, result);
    EXPECT_TRUE(agent_.create_datareader_by_ref(client_key_, datareader_id, subscriber_id, ref_one, flag, result));

    /*
     * Create DataReader over an existing with 0x00 flag.
     */
    EXPECT_FALSE(agent_.create_datareader_by_ref(client_key_, datareader_id, subscriber_id, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);
    EXPECT_FALSE(agent_.create_datareader_by_ref(client_key_, datareader_id, subscriber_id, ref_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);

    /*
     * Create DataReader over an existing with REUSE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE;
    EXPECT_TRUE(agent_.create_datareader_by_ref(client_key_, datareader_id, subscriber_id, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);
    EXPECT_FALSE(agent_.create_datareader_by_ref(client_key_, datareader_id, subscriber_id, ref_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::MISMATCH_ERROR);

    /*
     * Create DataReader over an existing with REPLACE flag.
     */
    flag = agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_datareader_by_ref(client_key_, datareader_id, subscriber_id, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);
    EXPECT_TRUE(agent_.create_datareader_by_ref(client_key_, datareader_id, subscriber_id, ref_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Create DataReader over an existing with REUSE & REPLACE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE | agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_datareader_by_ref(client_key_, datareader_id, subscriber_id, ref_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);
    EXPECT_TRUE(agent_.create_datareader_by_ref(client_key_, datareader_id, subscriber_id, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Delete DataReader.
     */
    EXPECT_TRUE(agent_.delete_datareader(client_key_, datareader_id, result));

    /*
     * Create DataReader with invalid REF.
     */
    EXPECT_FALSE(agent_.create_datareader_by_ref(client_key_, datareader_id, subscriber_id, "error", flag, result));
    // TODO (Julian): add when the reference data base is done.
    //EXPECT_EQ(errcode, agent_.ErrorCode::UNKNOWN_REFERENCE_ERRCODE);

    /*
     * Create DataReader with unknown Subscriber.
     */
    EXPECT_FALSE(agent_.create_datareader_by_ref(client_key_, datareader_id, 0x01, ref_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::UNKNOWN_REFERENCE_ERROR);
}


TEST_F(AgentUnitTests, CreateDataReaderByXml)
{
    Agent::OpResult result;
    agent_.create_client(client_key_, 0x01, 512, Middleware::Kind::FAST, result);

    const char* participant_ref = "default_xrce_participant";
    const char* topic_ref = "helloworld_topic";
    const char* subscriber_xml = "subscriber";
    const char* xml_one = "<dds>"
                              "<data_reader>"
                                  "<topic>"
                                      "<kind>NO_KEY</kind>"
                                      "<name>HelloWorldTopic</name>"
                                      "<dataType>HelloWorld</dataType>"
                                      "<historyQos>"
                                          "<kind>KEEP_LAST</kind>"
                                          "<depth>5</depth>"
                                      "</historyQos>"
                                  "</topic>"
                                  "<qos>"
                                      "<durability>"
                                          "<kind>TRANSIENT_LOCAL</kind>"
                                      "</durability>"
                                  "</qos>"
                              "</data_reader>"
                          "</dds>";
    const char* xml_two = "<dds>"
                              "<data_reader>"
                                  "<topic>"
                                      "<kind>NO_KEY</kind>"
                                      "<name>HelloWorldTopic</name>"
                                      "<dataType>HelloWorld</dataType>"
                                      "<historyQos>"
                                          "<kind>KEEP_LAST</kind>"
                                          "<depth>10</depth>"
                                      "</historyQos>"
                                  "</topic>"
                                  "<qos>"
                                      "<durability>"
                                          "<kind>TRANSIENT_LOCAL</kind>"
                                      "</durability>"
                                  "</qos>"
                              "</data_reader>"
                          "</dds>";

    const int16_t domain_id = 0x00;
    const uint16_t participant_id = 0x00;
    const uint16_t topic_id = 0x00;
    const uint16_t subscriber_id = 0x00;
    const uint16_t datareader_id = 0x00;

    uint8_t flag = 0x00;

    /*
     * Create DataReader.
     */
    agent_.create_participant_by_ref(client_key_, participant_id, domain_id, participant_ref, flag, result);
    agent_.create_topic_by_ref(client_key_, topic_id, participant_id, topic_ref, flag, result);
    agent_.create_subscriber_by_xml(client_key_, subscriber_id, participant_id, subscriber_xml, flag, result);
    EXPECT_TRUE(agent_.create_datareader_by_xml(client_key_, datareader_id, subscriber_id, xml_one, flag, result));

    /*
     * Create DataReader over an existing with 0x00 flag.
     */
    EXPECT_FALSE(agent_.create_datareader_by_xml(client_key_, datareader_id, subscriber_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);
    EXPECT_FALSE(agent_.create_datareader_by_xml(client_key_, datareader_id, subscriber_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::ALREADY_EXISTS_ERROR);

    /*
     * Create DataReader over an existing with REUSE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE;
    EXPECT_TRUE(agent_.create_datareader_by_xml(client_key_, datareader_id, subscriber_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);
    EXPECT_FALSE(agent_.create_datareader_by_xml(client_key_, datareader_id, subscriber_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::MISMATCH_ERROR);

    /*
     * Create DataReader over an existing with REPLACE flag.
     */
    flag = agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_datareader_by_xml(client_key_, datareader_id, subscriber_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);
    EXPECT_TRUE(agent_.create_datareader_by_xml(client_key_, datareader_id, subscriber_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Create DataReader over an existing with REUSE & REPLACE flag.
     */
    flag = agent_.CreationFlag::REUSE_MODE | agent_.CreationFlag::REPLACE_MODE;
    EXPECT_TRUE(agent_.create_datareader_by_xml(client_key_, datareader_id, subscriber_id, xml_two, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK_MATCHED);
    EXPECT_TRUE(agent_.create_datareader_by_xml(client_key_, datareader_id, subscriber_id, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::OK);

    /*
     * Delete DataReader.
     */
    EXPECT_TRUE(agent_.delete_datareader(client_key_, datareader_id, result));

    /*
     * Create DataReader with invalid REF.
     */
    EXPECT_FALSE(agent_.create_datareader_by_xml(client_key_, datareader_id, subscriber_id, "error", flag, result));
    // TODO (Julian): add when the reference data base is done.
    //EXPECT_EQ(errcode, agent_.ErrorCode::UNKNOWN_REFERENCE_ERRCODE);

    /*
     * Create DataReader with unknown Subscriber.
     */
    EXPECT_FALSE(agent_.create_datareader_by_xml(client_key_, datareader_id, 0x01, xml_one, flag, result));
    EXPECT_EQ(result, agent_.OpResult::UNKNOWN_REFERENCE_ERROR);
}


} // namespace testing
} // namespace uxr
} // namespace eprosima

int main(int args, char** argv)
{
    ::testing::InitGoogleTest(&args, argv);
    return RUN_ALL_TESTS();
}
