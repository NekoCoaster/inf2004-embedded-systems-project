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
#include "ws2812b_Rebuilt_v2.h"

// #define DEBUG

#pragma region Non-Sensor Related stuff that you probably wouldnt care about

#ifdef DEBUG
#define DEBUG_printf printf
#else
#define DEBUG_printf(...)
#endif

#define MQTT_BUFF_SIZE 1025 // 1024 + 1 for null terminator
#define MQTT_TOTAL_SUBS 2   // Number of topics to subscribe to

#pragma region MQTT and Network Utilities

static u32_t payload_total_len = 0;
static u8_t payload_buffer[MQTT_BUFF_SIZE];
static u8_t topic_buffer[MQTT_BUFF_SIZE];
static u8_t payload_cpy_index = 0;

static char topic_sub_list[MQTT_TOTAL_SUBS][MQTT_BUFF_SIZE] = {"MKPICO_LED_HEX", "EXTERNAL_LED_HEX"};

/**
 * @brief Processes incoming MQTT messages and performs actions based on topics and payloads.
 *
 * This function is responsible for handling incoming MQTT messages. It prints information about
 * the received message, including the topic and payload. Additionally, it performs specific actions
 * based on the topic and payload content. In this example, it checks for specific topics related to LED control
 * and sets the LED accordingly.
 */
static void process_incoming_message()
{
    printf("New MQTT message received!\n");
    printf("%s[%d]: %s\n", topic_buffer, payload_cpy_index, payload_buffer);
    // do stuff here. maybe use a switch case to handle different topics,
    // and then based on the topic, do different things based on the payload value
    // Here's an example of setting the onboard LED to a specific color

    if (strcmp(topic_buffer, "MKPICO_LED_HEX") == 0)
    {
        set_makerpico_led_hex(payload_buffer);
        show_makerpico_led();
    }
    else if (strcmp(topic_buffer, "EXTERNAL_LED_HEX") == 0)
    {
        set_all_external_leds_hex(payload_buffer);
        show_external_led();
    }
    else
    {
        printf("Unknown topic: %s\n", topic_buffer);
    }
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
 * @brief Callback function for handling MQTT payload data.
 *
 * This function is called when MQTT payload data is received. It appends the received
 * data to a buffer, tracks the total payload length, and processes the complete message
 * once the entire payload is received.
 *
 * @param arg     Pointer to user-defined argument (may be NULL).
 * @param data    Pointer to the received payload data.
 * @param len     Length of the received payload data.
 * @param flags   Flags indicating additional information about the payload.
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
 * @brief Prints an IPv4 address in human-readable format.
 *
 * This function takes a 32-bit IPv4 address as input and prints its individual octets
 * in the format "X.X.X.X", where X represents the decimal value of each octet.
 *
 * @param addr  32-bit unsigned integer representing the IPv4 address.
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

int main()
{
    stdio_init_all();
    ws2812b_init_all();

    set_makerpico_led_hex("0000FF");
    show_makerpico_led();

#pragma region WiFi setup
    printf("Build Version: 101. Press Resume on debugger to continue.\n");

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_SINGAPORE))
    {
        printf("Wi-Fi module failed to initialise\n");
        set_makerpico_led_hex("FF0000");
        show_makerpico_led();
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    set_makerpico_led_hex("FF00FF");
    show_makerpico_led();

    printf("Connecting to '%s' using '%s' \n", WIFI_SSID, WIFI_PASSWORD);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("Error connecting to Wi-Fi\n");
        set_makerpico_led_hex("FF0000");
        show_makerpico_led();
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
        set_makerpico_led_hex("FFFF00");
        show_makerpico_led();
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
    while (mqtt_begin_connection() != ERR_OK)
    {
        printf("Failed to connect to MQTT server. Retrying in 5 seconds...\n");
        set_makerpico_led_hex("FF0000");
        show_makerpico_led();
        sleep_ms(5000);
        set_makerpico_led_hex("FFFF00");
        show_makerpico_led();
    }
    printf("Connected to MQTT server.\n");
    set_mqtt_subscribe_callback(mqtt_notify, mqtt_read_payload, NULL);
    mqtt_publish_data(MQTT_CLIENT_ID, "ONLINE");
    for (int i = 0; i < MQTT_TOTAL_SUBS; i++)
    {
        mqtt_subscribe_topic(topic_sub_list[i], SUB);
    }
    set_makerpico_led_hex("00FF00");
    show_makerpico_led();
#pragma endregion
#pragma region Main loop
    while (1)
    {
        cyw43_arch_poll();
        sleep_ms(10);
    }
#pragma endregion
    cyw43_arch_deinit();
    printf("Bye!\n");
    return 0;
}
