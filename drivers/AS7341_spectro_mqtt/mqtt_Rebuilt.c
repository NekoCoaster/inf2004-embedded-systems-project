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

#define MQTT_MESSAGE_BUFFER_SIZE 1024

#define MQTT_BUFF_SIZE MQTT_MESSAGE_BUFFER_SIZE + 1

static u32_t payload_total_len = 0;
static u8_t payload_buffer[MQTT_BUFF_SIZE];
static u8_t payload_cpy_index = 0;
static bool ready_for_next_pubsub = true;
static mqtt_client_config_t mqtt_config;

static MQTT_CLIENT_T *_mqtt_state = NULL;

#define DEBUG

#ifdef DEBUG
#define DEBUG_printf printf
#else
#define DEBUG_printf(...)
#endif

#pragma region Network Tools
static void dns_found(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    MQTT_CLIENT_T *state = (MQTT_CLIENT_T *)callback_arg;
    DEBUG_printf("DNS query finished with resolved addr of %s.\n", ip4addr_ntoa(ipaddr));
    state->remote_addr = *ipaddr;
}

static void run_dns_lookup(MQTT_CLIENT_T *state, const char *mqtt_server_domain_name)
{
    DEBUG_printf("Running DNS query for %s.\n", mqtt_server_domain_name);

    cyw43_arch_lwip_begin();
    err_t err = dns_gethostbyname(mqtt_server_domain_name, &(state->remote_addr), dns_found, state);
    cyw43_arch_lwip_end();

    if (err == ERR_ARG)
    {
        DEBUG_printf("failed to start DNS query\n");
        return;
    }

    if (err == ERR_OK)
    {
        DEBUG_printf("no lookup needed");
        return;
    }

    while (state->remote_addr.addr == 0)
    {
        cyw43_arch_poll();
        sleep_ms(1);
    }
}

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

err_t mqtt_begin_connection()
{
    struct mqtt_connect_client_info_t ci;
    err_t err;

    memset(&ci, 0, sizeof(ci));

    ci.client_id = mqtt_config.client_id;
    ci.client_user = mqtt_config.client_user;
    ci.client_pass = mqtt_config.client_pass;
    ci.keep_alive = mqtt_config.keep_alive;
    ci.will_topic = mqtt_config.will_topic;
    ci.will_msg = mqtt_config.will_message;
    ci.will_qos = mqtt_config.will_qos;
    ci.will_retain = mqtt_config.retain_will;

    const struct mqtt_connect_client_info_t *client_info = &ci;

    err = mqtt_client_connect(_mqtt_state->mqtt_client, &(_mqtt_state->remote_addr), mqtt_config.server_port, mqtt_connection_cb, _mqtt_state, client_info);
    if (err != ERR_OK)
    {
        DEBUG_printf("mqtt_connect error code = %d\n", err);
    }
    else
    {
        DEBUG_printf("mqtt_connect success\n");
        mqtt_set_inpub_callback(_mqtt_state->mqtt_client, mqtt_incoming_notification_cb, mqtt_incoming_payload_cb, _mqtt_state);
    }
    return err;
}
#pragma endregion

#pragma region MQTT initation section

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

// Make sure that wifi is connected before calling this function
// Also make sure that the mqtt_config is set before calling this function
// Creates a new MQTT client and resolves the server address
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
static void mqtt_publish_data_cb(void *arg, err_t err)
{
    MQTT_CLIENT_T *state = (MQTT_CLIENT_T *)arg;
    if (err == ERR_OK)
    {
        ready_for_next_pubsub = true;
    }
    else
    {
        DEBUG_printf("Publish Error Code: %d\n", err);
    }
}

err_t mqtt_publish_data(const char *topic, const char *message)
{
    err_t err;

    // Wait for the previous pubsub to finish
    while (ready_for_next_pubsub == false)
    {
        cyw43_arch_poll();
        sleep_ms(1);
    }
    cyw43_arch_lwip_begin();
    ready_for_next_pubsub = false;
    err = mqtt_publish(_mqtt_state->mqtt_client, topic, message, strlen(message), mqtt_config.message_qos, mqtt_config.retain_messages, mqtt_publish_data_cb, _mqtt_state);
    cyw43_arch_lwip_end();
    if (err != ERR_OK)
    {
        DEBUG_printf("Publish err: %d\n", err);
    }
    return err;
}
#pragma endregion

bool readyForNextPubSub()
{
    return ready_for_next_pubsub;
}
#pragma region MQTT subscribe section

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

void set_mqtt_subscribe_callback(mqtt_incoming_publish_cb_t pub_cb, mqtt_incoming_data_cb_t data_cb, void *arg)
{
    mqtt_set_inpub_callback(_mqtt_state->mqtt_client, pub_cb, data_cb, arg);
}

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

err_t mqtt_subscribe_topic(const char *topic, eSubOrUnsub sub_or_unsub)
{

    err_t err;
    u8_t qos = mqtt_config.message_qos;
    // Wait for the previous pubsub to finish
    while (ready_for_next_pubsub == false)
    {
        cyw43_arch_poll();
        sleep_ms(1);
    }
    cyw43_arch_lwip_begin();
    ready_for_next_pubsub = false;
    err = mqtt_sub_unsub(_mqtt_state->mqtt_client, topic, mqtt_config.message_qos, mqtt_subscribe_topic_error_callback, _mqtt_state, sub_or_unsub);
    cyw43_arch_lwip_end();
    if (err != ERR_OK)
    {
        DEBUG_printf("Subscribe err: %d\n", err);
    }
    else if (sub_or_unsub == SUB)
    {
        DEBUG_printf("Subscribed to topic: %s\n", topic);
    }
    else if (sub_or_unsub == UNSUB)
    {
        DEBUG_printf("Unsubscribed from topic: %s\n", topic);
    }
    return err;
}

#pragma endregion

int getMqttBufferLength()
{
    return MQTT_BUFF_SIZE;
}