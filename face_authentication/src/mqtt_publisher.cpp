#include "mqtt_publisher.h"

MqttPublisher::MqttPublisher(const std::string& brokerAddress, int brokerPort, const std::string& clientId)
    : client_(brokerAddress, clientId)
{
    connOpts_.set_keep_alive_interval(20);
}

MqttPublisher::~MqttPublisher()
{
    if (client_.is_connected()) {
        disconnect();
    }
}

void MqttPublisher::connect()
{
    try {
        client_.connect(connOpts_)->wait();
    }
    catch (const mqtt::exception& exc) {
        throw std::runtime_error("Error connecting to MQTT broker: " + std::string(exc.what()));
    }
}

void MqttPublisher::disconnect()
{
    try {
        client_.disconnect()->wait();
    }
    catch (const mqtt::exception& exc) {
        throw std::runtime_error("Error disconnecting from MQTT broker: " + std::string(exc.what()));
    }
}

void MqttPublisher::publish(const std::string& topic, const std::string& message, int qos)
{
    mqtt::message_ptr pubmsg = mqtt::make_message(topic, message);
    pubmsg->set_qos(qos);

    try {
        client_.publish(pubmsg)->wait();
    }
    catch (const mqtt::exception& exc) {
        throw std::runtime_error("Error publishing MQTT message: " + std::string(exc.what()));
    }
}

