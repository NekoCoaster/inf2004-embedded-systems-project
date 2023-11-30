/** Disclaimer: While this code does uses the included mqtt client lirary that comes with the Pico SDK,
 * The example code that demonstrates how to use the included mqtt client library was written by Craig Niles (@cniles) from the following repository: https://github.com/cniles/picow-iot
 * While Craig Niles' example code was used as a reference for how to use the included mqtt client library in this project,
 * It has since then been heavily modified to fit the needs of this project.
 */

#include "hardware/structs/rosc.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include "mqtt_Rebuilt.h"

#define MQTT_MESSAGE_BUFFER_SIZE 1024 // Max size of a single MQTT message

#define MQTT_BUFF_SIZE MQTT_MESSAGE_BUFFER_SIZE + 1 // +1 for null terminator

static u32_t payload_total_len = 0;         // Total length of the incoming payload
static u8_t payload_buffer[MQTT_BUFF_SIZE]; // Buffer for the incoming payload
static u8_t payload_cpy_index = 0;          // Index of the payload buffer to copy the next incoming payload byte to
static bool ready_for_next_pubsub = true;   // Mutex variable to prevent new pub/subs from being called before the previous one is finished. Facilitates Atomicity.
static mqtt_client_config_t mqtt_config;    // MQTT configuration struct

static MQTT_CLIENT_T *_mqtt_state = NULL; // current MQTT state struct

#define DEBUG

// If DEBUG is defined, then printf statements will be enabled
// Otherwise, they will be disabled
//
#ifdef DEBUG
#define DEBUG_printf printf
#else
#define DEBUG_printf(...)
#endif

#pragma region Network Tools
/**
 * @brief Callback function for DNS resolution completion.
 *
 * This function is called when DNS resolution is completed.
 *
 * @param name - The domain name that was resolved.
 * @param ipaddr - Pointer to the resolved IP address structure (ip_addr_t).
 * @param callback_arg - Pointer to user-defined data passed during DNS query initiation.
 *
 * @note As this is a callback function used by dns_gethostbyname(), there is no reason this function
 * should be called by the user.
 */
static void dns_found(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    MQTT_CLIENT_T *state = (MQTT_CLIENT_T *)callback_arg;
    DEBUG_printf("DNS query finished with resolved addr of %s.\n", ip4addr_ntoa(ipaddr));
    state->remote_addr = *ipaddr;
}

/**
 * @brief Initiates DNS lookup for the given MQTT server domain name.
 *
 * This function runs a DNS query to resolve the IP address of the MQTT server domain name.
 * It utilizes asynchronous DNS resolution and a callback function for handling the result.
 *
 * @param state - Pointer to the MQTT client state structure.
 * @param mqtt_server_domain_name - The domain name of the MQTT server to look up.
 */
static void run_dns_lookup(MQTT_CLIENT_T *state, const char *mqtt_server_domain_name)
{
    DEBUG_printf("Running DNS query for %s.\n", mqtt_server_domain_name);

    cyw43_arch_lwip_begin();

    // Start DNS query, looks up the IP address specified by mqtt_server_domain_name.
    // If the IP address is already cached, the cached value is then stored in state->remote_addr.
    // Otherwise, query for it, then call dns_found() when the query is finished, passing in the mqtt client state structure
    // as the callback_arg, so that the state structure can be updated with the resolved IP address within the callback function.
    //
    err_t err = dns_gethostbyname(mqtt_server_domain_name, &(state->remote_addr), dns_found, state);
    cyw43_arch_lwip_end();

    if (err == ERR_ARG)
    {
        DEBUG_printf("failed to start DNS query\n");
        return;
    }

    // lets the cyw43 driver to do it's thing while waiting for the DNS query to finish.
    while (state->remote_addr.addr == 0)
    {
        cyw43_arch_poll();
        sleep_ms(1);
    }
}
/**
 * @brief Prints the IPv4 address in human-readable format.
 *
 * This function takes a 32-bit IPv4 address and prints it in the format "x.x.x.x".
 *
 * @param addr - 32-bit unsigned integer representing the IPv4 address.
 */
static void printIPv4Address(unsigned int addr)
{
    // Extracting individual octets
    unsigned char octet4 = (addr >> 24) & 0xFF;
    unsigned char octet3 = (addr >> 16) & 0xFF;
    unsigned char octet2 = (addr >> 8) & 0xFF;
    unsigned char octet1 = addr & 0xFF;

    // Printing the IPv4 address
    printf("%u.%u.%u.%u\n", octet1, octet2, octet3, octet4);
}

#pragma endregion

#pragma region MQTT utility functions

/**
 * @brief MQTT connection callback function.
 *
 * This function is called when the task of establishing an MQTT connection is completed, whether successful or not.
 * This function is then used to print a connection success or error message, depending on the mqtt_connection_status_t status.
 *
 * @param client - Pointer to the MQTT client structure.
 * @param arg - User-defined argument passed during MQTT client initialization.
 * @param status - MQTT connection status (0 for successful connection, non-zero for errors).
 */
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    if (status != 0)
    {
        DEBUG_printf("Error during connection: err %d.\n", status);
    }
    else
    {
        DEBUG_printf("MQTT connected.\n");
    }
}

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
err_t mqtt_begin_connection()
{
    struct mqtt_connect_client_info_t ci;
    err_t err;

    memset(&ci, 0, sizeof(ci)); // Zero out the client info struct

    // Set the client info struct with the configured parameters.
    ci.client_id = mqtt_config.client_id;
    ci.client_user = mqtt_config.client_user;
    ci.client_pass = mqtt_config.client_pass;
    ci.keep_alive = mqtt_config.keep_alive;
    ci.will_topic = mqtt_config.will_topic;
    ci.will_msg = mqtt_config.will_message;
    ci.will_qos = mqtt_config.will_qos;
    ci.will_retain = mqtt_config.retain_will;

    const struct mqtt_connect_client_info_t *client_info = &ci;

    // Connects to the MQTT broker.
    err = mqtt_client_connect(_mqtt_state->mqtt_client, &(_mqtt_state->remote_addr), mqtt_config.server_port, mqtt_connection_cb, _mqtt_state, client_info);
    if (err != ERR_OK)
    {
        DEBUG_printf("mqtt_connect error code = %d\n", err);
    }
    else
    {
        // If the connection is successful, bind the incoming message callback functions.
        mqtt_set_inpub_callback(_mqtt_state->mqtt_client, mqtt_incoming_notification_cb, mqtt_incoming_payload_cb, _mqtt_state);
    }
    return err;
}
#pragma endregion

#pragma region MQTT initation section
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
    uint16_t keep_alive,
    const char *will_topic,
    const char *will_message,
    u8_t will_qos,
    u8_t retain_will)
{

    memset(&mqtt_config, 0, sizeof(mqtt_config));
    mqtt_config.server_addr = server_addr;
    mqtt_config.server_port = server_port;
    mqtt_config.client_id = client_id;
    mqtt_config.client_user = client_user;
    mqtt_config.client_pass = client_pass;
    mqtt_config.retain_messages = retain_messages;
    mqtt_config.message_qos = message_qos;
    mqtt_config.keep_alive = keep_alive;
    mqtt_config.will_topic = will_topic;
    mqtt_config.will_message = will_message;
    mqtt_config.will_qos = will_qos;
    mqtt_config.retain_will = retain_will;
    printf("MQTT configured to connect to %s:%d\n", mqtt_config.server_addr, mqtt_config.server_port);
    printf("with client id: %s\n", mqtt_config.client_id);
    printf("with username and password: %s:%s\n", mqtt_config.client_user, mqtt_config.client_pass);
}

/**
 * @brief Initializes the MQTT client and performs DNS lookup.
 *
 * This function creates a new MQTT client, allocates memory for the MQTT client state,
 * and performs a DNS lookup to resolve the server address.
 *
 * @note requires that Wi-Fi is connected and the MQTT configuration is set before calling.
 */
void mqtt_client_init()
{
    _mqtt_state = calloc(1, sizeof(MQTT_CLIENT_T));
    if (!_mqtt_state)
    {
        DEBUG_printf("failed to allocate memory for MQTT state\n");
        return;
    }
    _mqtt_state->received = 0;
    _mqtt_state->counter = 0;
    _mqtt_state->mqtt_client = mqtt_client_new();
    if (!_mqtt_state->mqtt_client)
    {
        DEBUG_printf("failed to allocate memory for MQTT client\n");
        return;
    }
    DEBUG_printf("MQTT state allocated. Performing DNS Lookup...\n");
    run_dns_lookup(_mqtt_state, mqtt_config.server_addr);
    DEBUG_printf("DNS Lookup finished. IP address is: ");
#ifdef DEBUG
    printIPv4Address(_mqtt_state->remote_addr.addr);
#endif
}

#pragma endregion

#pragma region MQTT publish section
/**
 * @brief Callback function for MQTT data publishing.
 *
 * This function is called when the MQTT data publishing task is completed.
 * It sets a flag to indicate readiness for the next publish or subscribe operation.
 *
 * @param arg - Pointer to the MQTT client state structure.
 * @param err - Error code indicating the result of the publishing task.
 */
static void mqtt_publish_data_cb(void *arg, err_t err)
{
    MQTT_CLIENT_T *state = (MQTT_CLIENT_T *)arg;

    // Check if the publishing task was successful.
    if (err == ERR_OK)
    {
        // Unlock the ready_for_next_pubsub flag to allow the next pub/sub operation to be called.
        ready_for_next_pubsub = true;
    }
    else
    {
        // Print debug information in case of a publishing error.
        DEBUG_printf("Error Publishing MQTT message. err=%d\n", err);
    }
}

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
err_t mqtt_publish_data(const char *topic, const char *message)
{
    err_t err;

    // Wait for the previous pubsub operation to finish.
    while (!ready_for_next_pubsub)
    {
        cyw43_arch_poll();
        sleep_ms(1);
    }

    // Begin LWIP operations related to MQTT.
    cyw43_arch_lwip_begin();

    // locks ready_for_next_pubsub to prevent new pub/subs from being called before this publish transaction is finished.
    // This facilitates atomicity, and the lock will be released in the callback function on the condition that the publish transaction is successful.
    ready_for_next_pubsub = false;

    // Publish MQTT data using the configured parameters and callback function.
    err = mqtt_publish(_mqtt_state->mqtt_client, topic, message, strlen(message), mqtt_config.message_qos, mqtt_config.retain_messages, mqtt_publish_data_cb, _mqtt_state);

    // End LWIP operations related to MQTT.
    cyw43_arch_lwip_end();

    // Check for errors during publishing.
    /*Deprecated. This is now handled in the callback function.
    if (err != ERR_OK)
    {
        // Print debug information in case of a publishing error.
        DEBUG_printf("Publish err: %d\n", err);
    }
    */
    // Return the result of the publishing operation.
    return err;
}

#pragma endregion
/**
 * @brief Checks if the MQTT client is ready for the next pub/sub operation.
 *
 * This function returns a boolean value indicating whether the system is ready
 * for the next publish or subscribe operation. It checks the readiness flag.
 *
 * @return `True` if there are no currently running pub/sub operations, `False` otherwise.
 */
bool readyForNextPubSub()
{
    return ready_for_next_pubsub;
}
#pragma region MQTT subscribe section

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
static void mqtt_incoming_notification_cb(void *arg, const char *topic, u32_t tot_len)
{
    DEBUG_printf("Incoming publish at topic %s with total length %u\n", topic, (unsigned int)tot_len);
    if (tot_len > MQTT_BUFF_SIZE)
    {
        DEBUG_printf("Error: incoming publish does not fit in buffer. Data discarded\n");
        payload_total_len = 0;
    }
    else
    {
        payload_total_len = tot_len;
        payload_cpy_index = 0;
    }
}

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
static void mqtt_incoming_payload_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    if (payload_total_len > 0)
    {
        payload_total_len -= len;
        memcpy(&payload_buffer[payload_cpy_index], data, len);
        payload_cpy_index += len;

        if (payload_total_len == 0)
        {
            payload_buffer[payload_cpy_index] = 0;
            DEBUG_printf("Message Received [%d]:%s\n", payload_cpy_index, payload_buffer);
        }
    }
}

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
void set_mqtt_subscribe_callback(mqtt_incoming_publish_cb_t pub_cb, mqtt_incoming_data_cb_t data_cb, void *arg)
{
    mqtt_set_inpub_callback(_mqtt_state->mqtt_client, pub_cb, data_cb, arg);
}

/**
 * @brief Callback function for MQTT topic subscription and unsubscription.
 *
 * This function is called when the task of subscribing or unsubscribing from an MQTT topic is completed.
 * Also releases the ready_for_next_pubsub flag to allow the next pub/sub operation to be called if the task is successful.
 *
 * @param arg - Pointer to the MQTT client state structure.
 * @param err - Error code indicating the result of the subscribe or unsubscribe task.
 */
static void mqtt_subscribe_topic_error_callback(void *arg, err_t err)
{
    if (err == ERR_OK)
    {
        ready_for_next_pubsub = true;
    }
    else
    {
        DEBUG_printf("Subscribe Error Code: %d\n", err);
    }
}
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
err_t mqtt_subscribe_topic(const char *topic, eSubOrUnsub sub_or_unsub)
{
    err_t err;
    u8_t qos = mqtt_config.message_qos;

    // Wait for the previous pubsub operation to finish.
    while (ready_for_next_pubsub == false)
    {
        cyw43_arch_poll();
        sleep_ms(1);
    }

    // Begin LWIP operations related to MQTT.
    cyw43_arch_lwip_begin();

    // Set the flag to indicate the start of a new pubsub operation.
    ready_for_next_pubsub = false;

    // Subscribe or unsubscribe from the MQTT topic using the configured parameters and callback function.
    err = mqtt_sub_unsub(_mqtt_state->mqtt_client, topic, mqtt_config.message_qos, mqtt_subscribe_topic_error_callback, _mqtt_state, sub_or_unsub);

    // End LWIP operations related to MQTT.
    cyw43_arch_lwip_end();

    // Check for errors during subscribe or unsubscribe operation.
    if (err != ERR_OK)
    {
        // Print debug information in case of an error.
        DEBUG_printf("Subscribe err: %d\n", err);
        return err;
    }
    // Print debug information about successful subscription or unsubscription.
    if (sub_or_unsub == SUB)
    {
        DEBUG_printf("Subscribed to topic: %s\n", topic);
        return err;
    }

    if (sub_or_unsub == UNSUB)
    {
        DEBUG_printf("Unsubscribed from topic: %s\n", topic);
        return err;
    }
    return err; // I mean... it shouldn't get here, but just in case the pico gets hit with a bitflip or something by the higher beings...
}

#pragma endregion

/**
 * @brief Gets the MQTT buffer length.
 *
 * This function returns the configured length of the MQTT buffer.
 *
 * @return int - Length of the MQTT buffer.
 */
int getMqttBufferLength()
{
    return MQTT_BUFF_SIZE;
}