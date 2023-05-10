#ifndef MQTT_PUBLISHER_HPP
#define MQTT_PUBLISHER_HPP

#include <string>
#include <mqtt/async_client.h>

class MqttPublisher {
public:
    MqttPublisher(const std::string& brokerAddress, int brokerPort, const std::string& clientId);
    ~MqttPublisher();

    void connect();
    void disconnect();

    void publish(const std::string& topic, const std::string& message, int qos = 0);

private:
    mqtt::async_client client_;
    mqtt::connect_options connOpts_;
};

#endif // MQTT_PUBLISHER_HPP

