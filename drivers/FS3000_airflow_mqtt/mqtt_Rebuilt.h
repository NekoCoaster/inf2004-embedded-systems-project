

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

// When using this library, please setup the functions accordingly
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

void mqtt_client_init();
err_t mqtt_begin_connection();
err_t mqtt_publish_data(const char *topic, const char *message);
err_t mqtt_subscribe_topic(const char *topic, eSubOrUnsub sub_or_unsub);
void set_mqtt_subscribe_callback(mqtt_incoming_publish_cb_t pub_cb, mqtt_incoming_data_cb_t data_cb, void *arg);
int getMqttBufferLength();
bool readyForNextPubSub();
static void mqtt_incoming_notification_cb(void *arg, const char *topic, u32_t tot_len);
static void mqtt_incoming_payload_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);

#endif /* MQTT_REBUILT_H */