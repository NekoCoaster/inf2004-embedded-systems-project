#include "hardware/structs/rosc.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"

#include "inf2004_credentials.h"
#include "mqtt_Rebuilt.h"
#include "NFA4X10_Rebuilt.h"
#include "ws2812b_Rebuilt.h"

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

static int fan_speed = 100;
static int fan_speed_override = -1;

#define MQTT_TOTAL_SUB_TOPICS 4
/*IMPORTANT! REPLACE THIS*/

static char MQTT_SUB_TOPICS[MQTT_TOTAL_SUB_TOPICS][MQTT_BUFF_SIZE] = {
    MQTT_CLIENT_ID "/CMD",
    MQTT_CLIENT_ID "/DUTYCYCLE_OVERRIDE", // To override fan speed...
    MQTT_CLIENT_ID "/lightStatus",        // Override light status
    "INF2004_T09/inf2004_zh/AS7341/visibleLight"};

#pragma region MQTT incoming data functions

/**
 * @brief Trigger to process the message on the topic buffer and payload buffer...
 * (Conditional handling based on MQTT topic...)
 */
static void process_incoming_message()
{
    printf("New MQTT message received!\n");
    printf("%s[%d]: %s\n", topic_buffer, payload_cpy_index, payload_buffer);

    // NOTE: Sometimes there is a bug where the topic's title comes with a random character 't',
    // this is a 'hack' to just remove the 't' character if it comes with it ....
    const char *targetTopic = MQTT_CLIENT_ID "/DUTYCYCLE_OVERRIDEt ";
    if (strcmp(topic_buffer, targetTopic) == 0)
    {
        size_t length = strlen(topic_buffer);
        if (length > 0 && topic_buffer[length - 1] == 't')
        {
            topic_buffer[length - 1] = '\0';
        }
    }

    if (strcmp(topic_buffer, MQTT_SUB_TOPICS[1]) == 0)
    {
        // NOTE: MQTT Command to override fan speed...
        fan_speed_override = atoi(payload_buffer);
        printf("Override fan speed: %d\n", fan_speed_override);

        if (fan_speed_override >= 0 && fan_speed_override <= 100)
        {
            NFA4X10_set_fan_speed(fan_speed_override);
        }
    }
    else if (strcmp(topic_buffer, MQTT_SUB_TOPICS[2]) == 0)
    {
        // NOTE: MQTT Command to override light status...
        if (strcmp(topic_buffer, MQTT_SUB_TOPICS[2]) == 0)
        {

            if (strcmp(payload_buffer, "ON") == 0)
            {
                printf("Turning on light\n");
                set_all_external_leds_rgb(255, 255, 255);
                show_external_leds();
            }
            else if (strcmp(payload_buffer, "OFF") == 0)
            {
                printf("Turning off light\n");
                set_all_external_leds_rgb(0, 0, 0);
                show_external_leds();
            }
            else
            {
                printf("Invalid light status\n");
            }

            set_all_external_leds_rgb(255, 255, 255);
            show_external_leds();
        }
    }
    else if (strcmp(topic_buffer, MQTT_SUB_TOPICS[3]) == 0)
    {
        int ambientLightLevel = atoi(payload_buffer);
        if (ambientLightLevel < 500)
        {
            set_all_external_leds_rgb(255, 255, 255);
            show_external_leds();
        }
        else
        {
            set_all_external_leds_rgb(0, 0, 0);
            show_external_leds();
        }
    }
    else
    {
        printf("Topic Handler Not Yet Implemented For '%s'\n", topic_buffer);
    }

    /*TO-DO: Auto-calculate fan speed required based on new sensor readings from other clients*/
    // Placeholder code to simulate fan speed override
    if (fan_speed_override >= 0)
    {
        NFA4X10_set_fan_speed(fan_speed_override);
    }
    else if (fan_speed_override < 0)
    {
        NFA4X10_set_fan_speed(fan_speed);
    }
    else
    {
        NFA4X10_set_fan_speed(100);
    }
}

/**
NOTE: You'll need 2 functions to handle incoming messages
    1. mqtt_incoming_notification_cb
    This function is called when a new message is received.
    This function will tell you the topic and the total length of the message.

    2. mqtt_incoming_payload_cb
    This function will then be called only if the payload is not empty.
    This function will be called multiple times until the entire payload is received.
*/

/**
 * @brief Subcription function, sends the incoming MQTT Message to 'process_incoming_message'.
 * It will discord topic's messages that have a greater size than the buffer...
 */
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
        // since when there is no payload, the mqtt_read_payload() function will not be called
        // and thus the process_incoming_message() function will not be called as well
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

/**
 * @brief Subscribes to all predefined MQTT topics.
 */
static void mqtt_subscribe_to_all_topics()
{
    // Subscribe to all topics
    for (int i = 0; i < MQTT_TOTAL_SUB_TOPICS; ++i)
    {
        if (mqtt_subscribe_topic(MQTT_SUB_TOPICS[i], SUB) != ERR_OK)
        {
            printf("Failed to subscribe to topic: %s\n", MQTT_SUB_TOPICS[i]);
        }
    }
}

#pragma endregion

#pragma region MQTT publish section

/**
 * @brief Publishes sensor data in JSON format to MQTT.
 * @param sensorName: Name of the sensor.
 * @param valueNames: Array of value names.
 * @param sensorValues: Array of sensor values.
 * @param numValues: Number of values in the arrays.
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
 * @brief Publishes sensor data to MQTT.
 * @param sensorName: Name of the sensor.
 * @param JsonString: JSON-formatted sensor data.
 */
static void publishSensorData(const char *sensorName, const char *JsonString)
{
    char topic[MQTT_BUFF_SIZE];

    // Construct the topic: clientid/sensorname
    snprintf(topic, MQTT_BUFF_SIZE, "%s/%s", MQTT_CLIENT_ID, sensorName);
    /*legacy code. Fail safe has been implemented in mqtt_publish_data()
    while (!readyForNextPubSub())
    {
        cyw43_arch_poll();
    }
    */
    if (mqtt_publish_data(topic, JsonString) != ERR_OK)
    {
        printf("Failed to publish to topic: %s, message: %s\n", topic, JsonString);
        return;
    }
    printf("Published to topic: %s, message: %s\n", topic, JsonString);
}
#pragma endregion

/**
 * @brief Attempts to reconnect to the MQTT server.
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
 * @brief Attempts to reconnect to the MQTT server.
 */
void mqtt_reconnect()
{
    cyw43_arch_poll();
    while (mqtt_begin_connection() != ERR_OK)
    {
        printf("Failed to connect to MQTT server. Retrying in 5 seconds...\n");
        sleep_ms(5000);
    }
    printf("Connected to MQTT server.\n");
    // set our custom callback functions
    set_mqtt_subscribe_callback(mqtt_notify, mqtt_read_payload, NULL);
    // publish our online status
    mqtt_publish_data(MQTT_CLIENT_ID, "ONLINE");
    // subscribe to all topics
    mqtt_subscribe_to_all_topics();
}

/**
 * @brief Reads spectral data for sensors 1 to 8 and publishes it to MQTT based on a timer.
 */
void readSensorDataAndPublish()
{
    // Check if MQTT is connected by publishing "ONLINE" to the MQTT server
    if (mqtt_publish_data(MQTT_CLIENT_ID, "ONLINE") != ERR_OK)
    {
        printf("MQTT Server disconnected. Reconnecting...\n");
        mqtt_reconnect();
    }

    snprintf(MQTT_PUB_PAYLOAD_BUFFER, MQTT_BUFF_SIZE, "{\"RPM\":%.2f,\"DUTYCYCLE\":%d,\"DUTYCYCLE_OVERRIDE\":%d}",
             NFA4X10_get_fan_rpm(),
             NFA4X10_get_fan_duty_cycle(),
             fan_speed_override);

    publishSensorData("NFA4X10", MQTT_PUB_PAYLOAD_BUFFER);
}

int main()
{

    stdio_init_all();
    // Initializes the fan and it's components
    ws2812b_init_all();

    NFA4X10_init();
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
