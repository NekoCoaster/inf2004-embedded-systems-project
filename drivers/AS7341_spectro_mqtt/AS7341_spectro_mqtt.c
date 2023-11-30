/*This file is custom created to allow AS7341 to send data
* to be received by the mqtt. It includes the configurations 
* for the pico driver to connect, publish and subscribe to the mqtt.
*/

//include necessary header files
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
#include "AS7341_Rebuilt.h"
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

// Section for MQTT and Network Utilities.
#pragma region MQTT and Network Utilities

// Buffer for holding the MQTT publishing topic.
static char MQTT_PUB_TOPIC_BUFFER[MQTT_BUFF_SIZE];

// Buffer for holding the MQTT publishing payload.
static char MQTT_PUB_PAYLOAD_BUFFER[MQTT_BUFF_SIZE];

// Total length of the MQTT payload.
static u32_t payload_total_len = 0;

// Buffer to store MQTT payload data.
static u8_t payload_buffer[MQTT_BUFF_SIZE];

// Buffer to store MQTT topic data.
static u8_t topic_buffer[MQTT_BUFF_SIZE];

// Index variable for copying MQTT payload data.
static u8_t payload_cpy_index = 0;

// Total number of MQTT subscription topics.
#define MQTT_TOTAL_SUB_TOPICS 1

// Total number of MQTT publishing topics.
#define MQTT_TOTAL_PUB_TOPICS 1

// Array of MQTT subscription topics. Here, only one topic exists, formed by the MQTT client ID and "/CMD".
static char MQTT_SUB_TOPICS[MQTT_TOTAL_SUB_TOPICS][MQTT_BUFF_SIZE] = {MQTT_CLIENT_ID "/CMD"};

// Array of MQTT publishing topics. Here, only one topic exists, based on the MQTT client ID.
static char MQTT_PUB_TOPICS[MQTT_TOTAL_PUB_TOPICS][MQTT_BUFF_SIZE] = {MQTT_CLIENT_ID};

// Process the new MQTT message received
#pragma region MQTT incoming data functions
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

// Function to handle the MQTT payload received.
static void mqtt_read_payload(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    // Checking if there is still payload data to process.
    if (payload_total_len > 0)
    {
        // Decrementing the remaining length of the payload data by the received length.
        payload_total_len -= len;

        // Copying the received data into the payload buffer at the appropriate index.
        memcpy(&payload_buffer[payload_cpy_index], data, len);

        // Incrementing the payload copy index by the received length.
        payload_cpy_index += len;

        // Checking if the entire payload has been received.
        if (payload_total_len == 0)
        {
            // Null-terminating the payload buffer to indicate the end of the message.
            payload_buffer[payload_cpy_index] = 0;

            // Displaying a debug message indicating the received payload length and content.
            DEBUG_printf("Message Received [%d]:%s\n", payload_cpy_index, payload_buffer);

            // Calling the function to process the incoming MQTT message.
            process_incoming_message();
        }
    }
}

// Function to subscribe to all MQTT topics.
static void mqtt_subscribe_to_all_topics()
{
    // Looping through each MQTT subscription topic and subscribing to them.
    for (int i = 0; i < MQTT_TOTAL_SUB_TOPICS; i++)
    {
        // Subscribing to the MQTT topic and displaying the subscribed topic.
        mqtt_subscribe_topic(MQTT_SUB_TOPICS[i], SUB);
        printf("Subscribed to topic: %s\n", MQTT_SUB_TOPICS[i]);
    }
}
#pragma endregion



#pragma region MQTT publish section

// Function to format sensor data into JSON and publish it to MQTT
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

    // Publish the JSON message to the MQTT topic
    if (mqtt_publish_data(topic, jsonMessage) != ERR_OK)
    {
        printf("Failed to publish to topic: %s, message: %s\n", topic, jsonMessage);
        return;
    }
    printf("Published to topic: %s, message: %s\n", topic, jsonMessage);
}

// Function to publish sensor data to MQTT
static void publishSensorData(const char *sensorName, const char *JsonString)
{
    char topic[MQTT_BUFF_SIZE];

    // Construct the topic: clientid/sensorname
    snprintf(topic, MQTT_BUFF_SIZE, "%s/%s", MQTT_CLIENT_ID, sensorName);

    // Publish the JSON message to the MQTT topic
    if (mqtt_publish_data(topic, JsonString) != ERR_OK)
    {
        printf("Failed to publish to topic: %s, message: %s\n", topic, JsonString);
        return;
    }
    printf("Published to topic: %s, message: %s\n", topic, JsonString);
}

#pragma endregion

// Function to print an IPv4 address
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

// Function to read spectral data for sensors 1 to 4
AS7341_sModeOneData_t getSensor1to4()
{
    // Initiating the measurement for sensors 1 to 4
    AS7341_startMeasure(eF1F4ClearNIR);
    return AS7341_readSpectralDataOne();
}

// Function to read spectral data for sensors 5 to 8
AS7341_sModeTwoData_t getSensor5to8()
{
    // Initiating the measurement for sensors 5 to 8
    AS7341_startMeasure(eF5F8ClearNIR);
    return AS7341_readSpectralDataTwo();
}

// Function to reconnect to MQTT
void mqtt_reconnect()
{
    // Attempting to connect to the MQTT server continuously until success
    while (mqtt_begin_connection() != ERR_OK)
    {
        printf("Failed to connect to MQTT server. Retrying in 5 seconds...\n");
        sleep_ms(5000);
    }
    printf("Connected to MQTT server.\n");

    // Setting custom callback functions for MQTT
    set_mqtt_subscribe_callback(mqtt_notify, mqtt_read_payload, NULL);

    // Publishing online status to the MQTT server
    mqtt_publish_data(MQTT_PUB_TOPICS[0], "ONLINE");

    // Subscribing to all MQTT topics
    mqtt_subscribe_to_all_topics();
}

// Function to read spectral data for sensors 1 to 8 and publish to MQTT based on timer
void readSensorDataAndPublish()
{
    // Checking MQTT connection status by publishing "ONLINE" to the MQTT server
    if (mqtt_publish_data(MQTT_PUB_TOPICS[0], "ONLINE") != ERR_OK)
    {
        printf("MQTT Server disconnected. Reconnecting...\n");
        mqtt_reconnect();
    }

    // Reading spectral data for sensors 1 to 4 and 5 to 8
    AS7341_sModeOneData_t sensor1to4 = getSensor1to4();
    AS7341_sModeTwoData_t sensor5to8 = getSensor5to8();

    // Creating a JSON string with the sensor data
    snprintf(MQTT_PUB_PAYLOAD_BUFFER, MQTT_BUFF_SIZE, "{\"F1\":%d,\"F2\":%d,\"F3\":%d,\"F4\":%d,\"F5\":%d,\"F6\":%d,\"F7\":%d,\"F8\":%d,\"Visible\":%d,\"NIR\":%d}",
             sensor1to4.ADF1, sensor1to4.ADF2, sensor1to4.ADF3, sensor1to4.ADF4,
             sensor5to8.ADF5, sensor5to8.ADF6, sensor5to8.ADF7, sensor5to8.ADF8,
             sensor5to8.ADCLEAR, sensor5to8.ADNIR);

    // Publishing sensor data for sensors 1 to 8
    publishSensorData("AS7341", MQTT_PUB_PAYLOAD_BUFFER);

    // Formatting and publishing visible light sensor data separately
    snprintf(MQTT_PUB_PAYLOAD_BUFFER, MQTT_BUFF_SIZE, "%d", sensor1to4.ADCLEAR);
    publishSensorData("AS7341/visibleLight", MQTT_PUB_PAYLOAD_BUFFER);
}





int main()
{
    // Initializing standard input and output
    stdio_init_all();

    // Initializing I2C tools
    i2c_tools_init(i2c0, PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN);

    // Initiating AS7341 sensor
    AS7341_begin(eSpm);

    // Reading sensor ID
    int sensorid = AS7341_readID();

    // Checking if AS7341 sensor is connected
    while (sensorid == 0)
    {
        printf("AS7341 Sensor Not Connected. Please check the connection.\n");
        sleep_ms(1000);
        sensorid = AS7341_readID();
    }

    // Displaying AS7341 sensor connection status
    printf("AS7341 Sensor Connected. id=%d\n", sensorid);

#pragma region WiFi setup

    // Initializing Wi-Fi module with the specified country
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_SINGAPORE))
    {
        printf("Wi-Fi module failed to initialise\n");
        return 1;
    }

    // Enabling station mode for Wi-Fi
    cyw43_arch_enable_sta_mode();

    // Connecting to Wi-Fi using specified credentials
    printf("Connecting to '%s' using '%s' \n", WIFI_SSID, WIFI_PASSWORD);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("Error connecting to Wi-Fi\n");
        return 1;
    }
    else
    {
        printf("Connected to Wi-Fi.\n");
        // Starting LWIP for network connection
        cyw43_arch_lwip_begin();
        printf("Assigned IP address: ");
        // Printing the assigned IPv4 address
        printIPv4Address(cyw43_state.netif[0].ip_addr.addr);
        printf("\n");
        // Ending LWIP connection
        cyw43_arch_lwip_end();
    }
#pragma endregion

#pragma region MQTT setup

    // Setting up MQTT configuration
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

    // Initializing MQTT client
    mqtt_client_init();

    // Reconnecting to MQTT server
    mqtt_reconnect();

#pragma endregion

#pragma region Main loop

    // Variable to track the next time to read sensor data
    uint64_t nextTimeToReadSensor = 0;

    // Continuous loop for reading sensor data and publishing to MQTT
    while (1)
    {
        // Check if it's time to read sensor data based on interval
        if (nextTimeToReadSensor < time_us_64())
        {
            readSensorDataAndPublish(); // Reading sensor data and publishing to MQTT
            nextTimeToReadSensor = time_us_64() + SENSOR_READ_INTERVAL_MS * 1000; // Updating the next read time
        }
        cyw43_arch_poll(); // Polling the Wi-Fi
        sleep_ms(10); // Adding a small delay
    }

#pragma endregion

    cyw43_arch_deinit(); // Deinitializing the Wi-Fi module
    printf("Bye!\n"); // Displaying exit message
    return 0; // Exiting the program
}

