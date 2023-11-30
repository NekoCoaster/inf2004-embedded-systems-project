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

#include "inf2004_credentials.h"
#include "mqtt_Rebuilt.h"
#include "FS3000_Rebuilt.h"
#include "i2c_tools.h"

// #define DEBUG

#define SENSOR_READ_INTERVAL_MS 3000
#define MQTT_PUBLISH_WAIT_MS 100
#pragma region Non-Sensor Related stuff that you probably wouldnt care about

#ifdef DEBUG
#define DEBUG_printf printf
#else
#define DEBUG_printf(...)
#endif

#define MQTT_BUFF_SIZE 1025 // 1024 + 1 for null terminator

#pragma region MQTT and Network Utilities

static char MQTT_PUB_TOPIC_BUFFER[MQTT_BUFF_SIZE];
static char MQTT_PUB_PAYLOAD_BUFFER[MQTT_BUFF_SIZE];

static u32_t payload_total_len = 0;
static u8_t payload_buffer[MQTT_BUFF_SIZE];
static u8_t topic_buffer[MQTT_BUFF_SIZE];
static u8_t payload_cpy_index = 0;

#define MQTT_TOTAL_SUB_TOPICS 1
#define MQTT_TOTAL_PUB_TOPICS 1
static char MQTT_SUB_TOPICS[MQTT_TOTAL_SUB_TOPICS][MQTT_BUFF_SIZE] = {MQTT_CLIENT_ID "/CMD"};
static char MQTT_PUB_TOPICS[MQTT_TOTAL_PUB_TOPICS][MQTT_BUFF_SIZE] = {MQTT_CLIENT_ID};

#pragma region MQTT incoming data functions
/**
 * @brief Processes incoming MQTT messages.
 *
 * This function is responsible for handling incoming MQTT messages. It prints
 * information about the received message, such as the topic, payload index, and payload content.
 * After printing the message details, additional processing can be implemented based on the
 * topic and payload values.
 *
 * @param topic_buffer The buffer containing the topic of the received MQTT message.
 * @param payload_cpy_index The index indicating the position in the payload buffer where
 * the relevant data begins.
 * @param payload_buffer The buffer containing the payload of the received MQTT message.
 * The payload typically contains the actual data associated with the MQTT message.
 *
 * @return void
 */
static void process_incoming_message()
{
    printf("New MQTT message received!\n");
    printf("%s[%d]: %s\n", topic_buffer, payload_cpy_index, payload_buffer);
    // do stuff here. maybe use a switch case to handle different topics,
    // and then based on the topic, do different things based on the payload value
}
// You'll need 2 functions to handle incoming messages
// 1. mqtt_incoming_notification_cb
// 2. mqtt_incoming_payload_cb
// The first function is called when a new message is received.
// This function will tell you the topic and the total length of the message.
// The second function will then be called only if the payload is not empty.
// This function will be called multiple times until the entire payload is received.
//
static void mqtt_notify(void *arg, const char *topic, u32_t tot_len)
{
    DEBUG_printf("Incoming topic: '%s', total length: %u\n", topic, (unsigned int)tot_len);

    if (strlen(topic) > MQTT_BUFF_SIZE)
    {
        DEBUG_printf("Error: incoming topic does not fit in buffer. Data discarded\n");
        payload_total_len = 0;
        return;
    }
    if (tot_len > MQTT_BUFF_SIZE)
    {
        DEBUG_printf("Error: incoming payload does not fit in buffer. Data discarded\n");
        payload_total_len = 0;
        return;
    }
    memcpy(topic_buffer, topic, strlen(topic));
    payload_total_len = tot_len;
    payload_cpy_index = 0;
    if (payload_total_len == 0)
    {
        // If the payload is empty, then erase the payload buffer
        // And then call the process_incoming_message() function
        //  since when there is no payload, the mqtt_read_payload() function will not be called
        //  and thus the process_incoming_message() function will not be called as well
        // So we have to call it manually here instead.
        memset(payload_buffer, 0, MQTT_BUFF_SIZE);
        process_incoming_message();
    }
}

/**
 * @brief Callback function for reading MQTT payload data.
 *
 * This function is a callback used by the MQTT client to handle incoming payload data.
 * It is typically called as part of the MQTT message reception process. The function
 * updates the payload buffer with the received data and triggers further processing
 * once the entire payload is received.
 *
 * @param arg A pointer to user-defined data. In most cases, this is used to pass
 * context information to the callback function.
 * 
 * @param data A pointer to the received payload data.
 * 
 * @param len The length of the received payload data.
 * 
 * @param flags Flags providing additional information about the received payload.
 * 
 * @return void
 */
static void mqtt_read_payload(void *arg, const u8_t *data, u16_t len, u8_t flags)
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
            process_incoming_message();
        }
    }
}

/**
 * @brief Subscribe to all predefined MQTT topics.
 *
 * This function iterates through an array of MQTT subscription topics
 * and subscribes to each one using the `mqtt_subscribe_topic` function.
 * After each successful subscription, a message is printed to indicate
 * that the subscription to a specific topic has been completed.
 *
 * @return void
 */
static void mqtt_subscribe_to_all_topics()
{
    for (int i = 0; i < MQTT_TOTAL_SUB_TOPICS; i++)
    {
        mqtt_subscribe_topic(MQTT_SUB_TOPICS[i], SUB);
        printf("Subscribed to topic: %s\n", MQTT_SUB_TOPICS[i]);
    }
}

#pragma endregion

#pragma region MQTT publish section

/**
 * @brief Publishes sensor data formatted as JSON to an MQTT topic.
 *
 * This function constructs an MQTT topic based on the MQTT client ID and the
 * provided sensor name. It then formats the sensor data as a JSON message
 * using the provided arrays of value names and sensor values. The resulting
 * JSON message is printed or sent using the `mqtt_publish_data` function.
 *
 * @param sensorName The name of the sensor for which data is being published.
 * 
 * @param valueNames An array containing the names of the sensor values.
 * 
 * @param sensorValues An array containing the corresponding sensor values.
 * 
 * @param numValues The number of values in the `valueNames` and `sensorValues` arrays.
 * 
 * @return void
 */
static void publishSensorDataFormatToJson(const char *sensorName, char *valueNames[], char *sensorValues[], size_t numValues)
{
    char topic[MQTT_BUFF_SIZE];
    char jsonMessage[MQTT_BUFF_SIZE];

    // Construct the topic: clientid/sensorname
    snprintf(topic, MQTT_BUFF_SIZE, "%s/%s", MQTT_CLIENT_ID, sensorName);

    // Construct the JSON message
    snprintf(jsonMessage, MQTT_BUFF_SIZE, "{");

    for (size_t i = 0; i < numValues; ++i)
    {
        // Add the valueName and sensorValue to the JSON message
        strcat(jsonMessage, "\"");
        strcat(jsonMessage, valueNames[i]);
        strcat(jsonMessage, "\":\"");
        strcat(jsonMessage, sensorValues[i]);
        strcat(jsonMessage, "\"");

        // Add a comma if not the last element
        if (i < numValues - 1)
        {
            strcat(jsonMessage, ",");
        }
    }

    strcat(jsonMessage, "}");

    // Print or send the JSON message
    printf("Topic: %s\n", topic);
    printf("JSON Message: %s\n", jsonMessage);

    if (mqtt_publish_data(topic, jsonMessage) != ERR_OK)
    {
        printf("Failed to publish to topic: %s, message: %s\n", topic, jsonMessage);
        return;
    }
    printf("Published to topic: %s, message: %s\n", topic, jsonMessage);
}

/**
 * @brief Publishes sensor data in JSON format to an MQTT topic.
 *
 * This function constructs an MQTT topic based on the MQTT client ID and the
 * provided sensor name. It then publishes the provided JSON-formatted sensor
 * data to the constructed topic using the `mqtt_publish_data` function.
 *
 * @param sensorName The name of the sensor for which data is being published.
 * 
 * @param JsonString A JSON-formatted string containing the sensor data to be published.
 * 
 * @return void
 */
static void publishSensorData(const char *sensorName, const char *JsonString)
{
    char topic[MQTT_BUFF_SIZE];

    // Construct the topic: clientid/sensorname
    snprintf(topic, MQTT_BUFF_SIZE, "%s/%s", MQTT_CLIENT_ID, sensorName);
    
    // Publish the JSON-formatted sensor data to the MQTT topic
    if (mqtt_publish_data(topic, JsonString) != ERR_OK)
    {
        printf("Failed to publish to topic: %s, message: %s\n", topic, JsonString);
        return;
    }
    
    printf("Published to topic: %s, message: %s\n", topic, JsonString);
}
#pragma endregion
/**
 * @brief Prints an IPv4 address in human-readable format.
 *
 * This function takes a 32-bit unsigned integer representing an IPv4 address
 * and prints it in the standard dot-decimal notation (e.g., "192.168.0.1").
 * The address is divided into individual octets, and each octet is printed
 * in decimal format.
 *
 * @param addr The 32-bit unsigned integer representing the IPv4 address.
 * 
 * @return void
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

#pragma endregion
/**
 * @brief Reconnects to the MQTT server, sets custom callbacks, publishes online status, and subscribes to topics.
 *
 * This function attempts to establish a connection to the MQTT server using the `mqtt_begin_connection` function.
 * If the connection attempt fails, it retries every 5 seconds until successful. Once connected, it sets custom
 * MQTT callback functions using `set_mqtt_subscribe_callback`, publishes an online status message to a predefined
 * MQTT topic using `mqtt_publish_data`, and subscribes to all predefined MQTT topics using `mqtt_subscribe_to_all_topics`.
 *
 * @return void
 */
void mqtt_reconnect()
{
    while (mqtt_begin_connection() != ERR_OK)
    {
        printf("Failed to connect to MQTT server. Retrying in 5 seconds...\n");
        sleep_ms(5000);
    }
    
    printf("Connected to MQTT server.\n");

    // set our custom callback functions
    set_mqtt_subscribe_callback(mqtt_notify, mqtt_read_payload, NULL);

    // publish our online status
    mqtt_publish_data(MQTT_PUB_TOPICS[0], "ONLINE");

    // subscribe to all topics
    mqtt_subscribe_to_all_topics();
}

// Function to read spectral data for sensors 1 to 8 and publish to MQTT, based on timer
/**
 * @brief Reads sensor data from an FS3000 sensor and publishes it to the MQTT server.
 *
 * This function first checks if the MQTT server is connected by publishing an "ONLINE" status message
 * to a predefined MQTT topic. If the MQTT server is disconnected, it attempts to reconnect using the
 * `mqtt_reconnect` function. After ensuring a connection, it reads sensor data from an FS3000 sensor
 * and formats it into a JSON payload. The payload is then published to a predefined MQTT topic using
 * the `publishSensorData` function.
 *
 * @return void
 */
void readSensorDataAndPublish()
{
    // Check if MQTT is connected by publishing "ONLINE" to the MQTT server
    if (mqtt_publish_data(MQTT_PUB_TOPICS[0], "ONLINE") != ERR_OK)
    {
        printf("MQTT Server disconnected. Reconnecting...\n");
        mqtt_reconnect();
    }

    // Read sensor data from FS3000 and format it into a JSON payload
    snprintf(MQTT_PUB_PAYLOAD_BUFFER, MQTT_BUFF_SIZE, "{\"RAW\":%d,\"metersPerSec\":%.2f,\"milesPerHour\":%.2f}",
             FS3000_readRaw(),
             FS3000_readMetersPerSecond(),
             FS3000_readMilesPerHour());

    // Publish the sensor data to the MQTT server
    publishSensorData("FS3000", MQTT_PUB_PAYLOAD_BUFFER);
}

int main()
{

    stdio_init_all();
    i2c_tools_init(i2c0, PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN);
    i2c_tools_begin();

    if (!FS3000_begin())
    {
        printf("FS3000 Not Detected. Please check wiring! Retrying in 3 seconds...\n");
        sleep_ms(1000);
    }
    FS3000_setRange(AIRFLOW_RANGE_15_MPS);
#pragma region WiFi setup

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_SINGAPORE))
    {
        printf("Wi-Fi module failed to initialise\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Connecting to '%s' using '%s' \n", WIFI_SSID, WIFI_PASSWORD);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("Error connecting to Wi-Fi\n");
        return 1;
    }
    else
    {
        printf("Connected to Wi-Fi.\n");
        cyw43_arch_lwip_begin();
        printf("Assigned IP address: ");
        printIPv4Address(cyw43_state.netif[0].ip_addr.addr);
        printf("\n");
        cyw43_arch_lwip_end();
    }
#pragma endregion
#pragma region MQTT setup
    set_mqtt_config(
        MQTT_SERVER_ADDR,
        MQTT_SERVER_PORT,
        MQTT_CLIENT_ID,
        MQTT_USERNAME,
        MQTT_PASSWORD,
        MQTT_RETAIN_ALL_MESSAGES,
        MQTT_QOS,
        MQTT_KEEP_ALIVE,
        MQTT_WILL_TOPIC,
        MQTT_WILL_MESSAGE,
        MQTT_WILL_QOS,
        MQTT_WILL_RETAIN);

    mqtt_client_init();
    mqtt_reconnect();

#pragma endregion

#pragma region Main loop

    uint64_t nextTimeToReadSensor = 0;
    while (1)
    {
        if (nextTimeToReadSensor < time_us_64())
        {
            readSensorDataAndPublish();
            nextTimeToReadSensor = time_us_64() + SENSOR_READ_INTERVAL_MS * 1000;
        }
        cyw43_arch_poll();
        sleep_ms(10);
    }
#pragma endregion
    cyw43_arch_deinit();
    printf("Bye!\n");
    return 0;
}
