// 1. RENAME THIS TO inf2004_credentials.h
// 2. FILL IN YOUR CREDENTIALS BELOW
// 3. PUT THIS FILE IN THE SAME FOLDER AS YOUR MAIN PROGRAM

#ifndef INF2004_CREDENTIALS_H
#define INF2004_CREDENTIALS_H

// Define your Wi-Fi SSID and password
// mobile hotspot is recommended as normal Wi-Fi APs do not play nicely with constant reconnects
#define WIFI_SSID "YourWifiSSID"
#define WIFI_PASSWORD "YourWifiPassword"

// Define your IoT service credentials
#define MQTT_SERVER_ADDR "YourMqttServerAddress"
#define MQTT_SERVER_PORT 1883
#define MQTT_USERNAME "YourMqttUsername"
#define MQTT_PASSWORD "YourMqttPassword"
#define MQTT_CLIENT_ID "YourGroupName/" MQTT_USERNAME // Creates a unique MQTT client ID based on your group name and username (Also groups your MQTT topics together that shows up nicely in MQTT Explorer)
/*Do note that if any 2 clients have the same client ID (and potentially username as well), only one of them will be able to connect to the MQTT broker.*/
/*If both clients with the same client ID are connected, the one that connected first will be disconnected.*/

// Additional MQTT settings not including clean session. Leave as default if unsure.
#define MQTT_RETAIN_ALL_MESSAGES 1 // Retains all messages on the MQTT broker, so when a new client connects, it will receive all messages that were sent to the topic while it was offline.
#define MQTT_QOS 1
#define MQTT_KEEP_ALIVE 60
/*MQTT Will Messages and Topics Explanation:
In MQTT, a Will Message is a crucial feature that allows a client to specify a message that should be sent by the broker in case the client unexpectedly disconnects.
This is particularly useful for scenarios where the client may go offline without sending a proper disconnect message.
Usage:
1. 'MQTT_WILL_TOPIC' and 'MQTT_WILL_MESSAGE' will be used during initializing of the MQTT client to define the topic and message for the will.
2. If the client disconnects unexpectedly (due to a network issue, for example), the broker will publish the specified 'MQTT_WILL_MESSAGE' to the 'MQTT_WILL_TOPIC'.
3. This helps other subscribers on the MQTT network to be informed about the unexpected disconnect, enabling them to take appropriate actions or make decisions based on this information.
As a bonus, this also provides a nice overview in MQTT Explorer of which clients are online and offline.
*/
#define MQTT_WILL_TOPIC MQTT_CLIENT_ID
#define MQTT_WILL_MESSAGE "OFFLINE"
#define MQTT_WILL_QOS 1
#define MQTT_WILL_RETAIN 1

#endif // CREDENTIALS_H
