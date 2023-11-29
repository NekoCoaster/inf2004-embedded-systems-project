#include "hardware/structs/rosc.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"

#include "inf2004_credentials.h"
#include "mqtt_Rebuilt.h"

#include "scd4x_i2c.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"

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

static void publishSensorData(const char *sensorName, const char *JsonString)
{
    char topic[MQTT_BUFF_SIZE];

    // Construct the topic: clientid/sensorname
    snprintf(topic, MQTT_BUFF_SIZE, "%s/%s", MQTT_CLIENT_ID, sensorName);
    if (mqtt_publish_data(topic, JsonString) != ERR_OK)
    {
        printf("Failed to publish to topic: %s, message: %s\n", topic, JsonString);
        return;
    }
    printf("Published to topic: %s, message: %s\n", topic, JsonString);
}
#pragma endregion
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
void readSensorDataAndPublish()
{
    bool data_ready_flag = false;
    int16_t error = 0;
    error = scd4x_get_data_ready_flag(&data_ready_flag);
    if (error)
    {
        printf("Error executing scd4x_get_data_ready_flag(): %i\n", error);
        return;
    }
    if (data_ready_flag)
    {
        uint16_t co2;
        int32_t temperature;
        int32_t humidity;
        error = scd4x_read_measurement(&co2, &temperature, &humidity);
        if (error)
        {
            printf("Error executing scd4x_read_measurement(): %i\n", error);
            return;
        }
        if (co2 == 0)
        {
            printf("Invalid sample detected, skipping.\n");
            return;
        }
        // Check if MQTT is connected by publishing "ONLINE" to the MQTT server
        if (mqtt_publish_data(MQTT_PUB_TOPICS[0], "ONLINE") != ERR_OK)
        {
            printf("MQTT Server disconnected. Reconnecting...\n");
            mqtt_reconnect();
        }

        // Publish the CO2, temperature and humidity to MQTT
        snprintf(MQTT_PUB_PAYLOAD_BUFFER, MQTT_BUFF_SIZE, "{\"CO2\":%u,\"Temperature\":%d,\"Humidity\":%d}",
                 co2,
                 temperature,
                 humidity);

        publishSensorData("SCD41", MQTT_PUB_PAYLOAD_BUFFER);
    }
}

int main()
{
    int16_t error = 0;
    stdio_init_all();
    sensirion_i2c_hal_init();

    // Clean up potential SCD40 states
    scd4x_wake_up();
    scd4x_stop_periodic_measurement();
    scd4x_reinit();

    uint16_t serial_0;
    uint16_t serial_1;
    uint16_t serial_2;
    error = scd4x_get_serial_number(&serial_0, &serial_1, &serial_2);
    if (error)
    {
        printf("Error executing scd4x_get_serial_number(): %i\n", error);
    }
    else
    {
        printf("serial: 0x%04x%04x%04x\n", serial_0, serial_1, serial_2);
    }

    // Start Measurement

    error = scd4x_start_periodic_measurement();
    if (error)
    {
        printf("Error executing scd4x_start_periodic_measurement(): %i\n",
               error);
    }
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
