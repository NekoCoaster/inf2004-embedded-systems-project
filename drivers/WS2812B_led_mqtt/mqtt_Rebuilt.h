/** @file mqtt_Rebuilt.h
 *
 * Disclaimer: While this code does uses the included mqtt client lirary that comes with the Pico SDK,
 * The example code that demonstrates how to use the included mqtt client library was written by Craig Niles (@cniles) from the following repository: https://github.com/cniles/picow-iot
 * While Craig Niles' example code was used as a reference for how to use the included mqtt client library in this project,
 * It has since then been heavily modified to fit the needs of this project.
 */

#ifndef MQTT_REBUILT_H
#define MQTT_REBUILT_H

#include "pico/stdlib.h"

typedef struct MQTT_CLIENT_T_
{
    ip_addr_t remote_addr;
    mqtt_client_t *mqtt_client;
    u32_t received;
    u32_t counter;
    u32_t reconnect;
} MQTT_CLIENT_T;

typedef struct MQTT_CLIENT_CONFIG_T_
{
    // Pre-connection settings
    const char *server_addr;
    u16_t server_port;
    const char *client_id;
    const char *client_user;
    const char *client_pass;
    // Running settings
    u8_t retain_messages;
    u8_t message_qos;
    // Post-connection settings
    uint16_t keep_alive;
    const char *will_topic;
    const char *will_message;
    u8_t will_qos;
    u8_t retain_will;

} mqtt_client_config_t;

typedef enum SUB_OR_UNSUB
{
    UNSUB,
    SUB
} eSubOrUnsub;

/**
 * @brief Configures MQTT settings for connection.
 *
 * This function sets up MQTT configuration parameters for establishing a connection.
 * It initializes pre-connection, running, and post-connection settings.
 *
 * @param server_addr - MQTT broker server address.
 * @param server_port - MQTT broker server port.
 * @param client_id - Client identifier for the MQTT connection.
 * @param client_user - Username for MQTT authentication.
 * @param client_pass - Password for MQTT authentication.
 * @param retain_messages - Retain flag for published messages.
 * @param message_qos - Quality of Service level for published messages.
 * @param keep_alive - Keep-alive interval for the MQTT connection.
 * @param will_topic - Topic for the Last Will and Testament.
 * @param will_message - Message for the Last Will and Testament.
 * @param will_qos - Quality of Service level for the Last Will and Testament.
 * @param retain_will - Retain flag for the Last Will and Testament.
 *
 * @note (No notes here, just padding the Hint Popup.
 * @note in case the number of parameters gets cuts off in the popup window due to the number of parameters).
 * @note ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 */
void set_mqtt_config(
    // Pre-connection settings
    const char *server_addr,
    u16_t server_port,
    const char *client_id,
    const char *client_user,
    const char *client_pass,
    // Running settings
    u8_t retain_messages,
    u8_t message_qos,
    // Post-connection settings
    uint16_t keepalive,
    const char *will_topic,
    const char *will_message,
    u8_t will_qos,
    u8_t retain_will);

/**
 * @brief Initializes the MQTT client and performs DNS lookup.
 *
 * This function creates a new MQTT client, allocates memory for the MQTT client state,
 * and performs a DNS lookup to resolve the server address.
 *
 * @note requires that Wi-Fi is connected and the MQTT configuration is set before calling.
 */
void mqtt_client_init();

/**
 * @brief Initiates the MQTT connection using provided configuration.
 *
 * This function sets up and begins the MQTT connection using the configured parameters.
 * It establishes a connection to the MQTT broker and sets callback functions for handling incoming messages.
 *
 * @return err_t - ERR_OK if the connection initiation is successful, an error code otherwise.
 *
 * @note Before calling this function, please ensure that the following has been performed in the following order:
 * @note 1. WiFi connection is established and the MQTT configuration is set. (see set_mqtt_config())
 * @note 2. mqtt_client_init() called. (for initializing the MQTT client, performing DNS lookup, etc. All the good stuff.)
 */
err_t mqtt_begin_connection();

/**
 * @brief Publishes MQTT data to the specified topic.
 *
 * This function publishes MQTT data to the specified topic using the configured MQTT client.
 * It waits for the previous pubsub operation to finish before initiating a new one.
 *
 * @param topic - MQTT topic to which the data will be published.
 * @param message - Message data to be published.
 * @return err_t - ERR_OK if the publishing is successful, an error code otherwise.
 */
err_t mqtt_publish_data(const char *topic, const char *message);

/**
 * @brief Subscribes or unsubscribes from an MQTT topic.
 *
 * This function subscribes or unsubscribes from the specified MQTT topic using the configured MQTT client.
 * It waits for the previous pubsub operation to finish before initiating a new one.
 *
 * @param topic - MQTT topic to subscribe or unsubscribe from.
 * @param sub_or_unsub - Enumeration specifying whether to subscribe (SUB) or unsubscribe (UNSUB).
 * @return err_t - ERR_OK if the subscribe or unsubscribe operation is successful, an error code otherwise.
 */
err_t mqtt_subscribe_topic(const char *topic, eSubOrUnsub sub_or_unsub);

/**
 * @brief Sets the callback functions for incoming MQTT publish notifications and payloads.
 *
 * This function allows the user to set custom callback functions for processing incoming MQTT publish notifications and payloads.
 * If not set, the default callback functions will be used instead, which simply prints debug information about the incoming message.
 * Otherwise, if set, the user-defined callback functions will be used instead.
 *
 * @param pub_cb - Callback function for processing new incoming message topics and information about it's payload.
 * @param data_cb - Callback function for incoming message payload.
 * @param arg - Any additional arguments to be passed to the callback functions.
 *
 * @note When a message is recieved, pub_cb is called first followed by data_cb.
 * @note And if there is no payload, (e.g. a topic was published to the broker without any message), only pub_cb will be called but data_cb will not be called.
 * @note ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
void set_mqtt_subscribe_callback(mqtt_incoming_publish_cb_t pub_cb, mqtt_incoming_data_cb_t data_cb, void *arg);

/**
 * @brief Gets the MQTT buffer length.
 *
 * This function returns the configured length of the MQTT buffer.
 *
 * @return int - Length of the MQTT buffer.
 */
int getMqttBufferLength();

/**
 * @brief Checks if the MQTT client is ready for the next pub/sub operation.
 *
 * This function returns a boolean value indicating whether the system is ready
 * for the next publish or subscribe operation. It checks the readiness flag.
 *
 * @return `True` if there are no currently running pub/sub operations, `False` otherwise.
 */
bool readyForNextPubSub();

/**
 * @brief Callback function for incoming MQTT publish notifications.
 *
 * When an incoming publish is received, this function is called to process information about the incoming message.
 * This includes the topic of the message, and as well as the total length of the payload.
 *
 * We then use this information to check if the incoming payload will fit in our processing buffer.
 * and if it does, we set the payload_total_len variable to the total length of the incoming payload,
 * and reset the payload_cpy_index variable to 0, which is the index of the payload buffer to copy the next incoming payload byte to.
 *
 * Otherwise, if the incoming payload does not fit in our processing buffer, we set the payload_total_len variable to 0,
 * and discard the incoming payload.
 *
 * @param arg - Pointer to the MQTT client state structure.
 * @param topic - MQTT topic to which the incoming publish is sent.
 * @param tot_len - Total length of the incoming payload.
 *
 * @note If the incoming message does not contain a payload, mqtt_incoming_payload_cb() will not be called.
 * @note ~~
 */
static void mqtt_incoming_notification_cb(void *arg, const char *topic, u32_t tot_len);

/**
 * @brief Callback function for incoming MQTT publish payloads.
 *
 * When an incoming message is received, this function is called to process the payload of the incoming message.
 * This function copies the incoming payload byte by byte into the payload buffer.
 *
 * @param arg - Pointer to the MQTT client state structure.
 * @param data - Pointer to the incoming payload data.
 * @param len - Length of the incoming payload.
 * @param flags - Flags indicating the status of the incoming payload.
 *
 * @note If the incoming message does not contain a payload, this function will not be called.
 * @note ~~
 */
static void mqtt_incoming_payload_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);

#endif /* MQTT_REBUILT_H */