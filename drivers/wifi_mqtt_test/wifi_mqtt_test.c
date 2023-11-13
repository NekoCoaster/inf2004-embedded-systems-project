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

// #define CYW43_HOST_NAME "INF2004 P9 PICO W"
#define DEBUG_printf printf

// MQTT Client code by Craig Niles https://github.com/cniles/picow-iot/blob/main/picow_iot.c
typedef struct MQTT_CLIENT_T_
{
    ip_addr_t remote_addr;
    mqtt_client_t *mqtt_client;
    u32_t received;
    u32_t counter;
    u32_t reconnect;
} MQTT_CLIENT_T;

err_t mqtt_test_connect(MQTT_CLIENT_T *state);

// Perform initialisation
static MQTT_CLIENT_T *mqtt_client_init(void)
{
    MQTT_CLIENT_T *state = calloc(1, sizeof(MQTT_CLIENT_T));
    if (!state)
    {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    state->received = 0;
    return state;
}

void dns_found(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    MQTT_CLIENT_T *state = (MQTT_CLIENT_T *)callback_arg;
    DEBUG_printf("DNS query finished with resolved addr of %s.\n", ip4addr_ntoa(ipaddr));
    state->remote_addr = *ipaddr;
}

void run_dns_lookup(MQTT_CLIENT_T *state)
{
    DEBUG_printf("Running DNS query for %s.\n", MQTT_SERVER_ADDR);

    cyw43_arch_lwip_begin();
    err_t err = dns_gethostbyname(MQTT_SERVER_ADDR, &(state->remote_addr), dns_found, state);
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

u32_t data_in = 0;

u8_t buffer[1025];
u8_t data_len = 0;

static void mqtt_pub_start_cb(void *arg, const char *topic, u32_t tot_len)
{
    DEBUG_printf("mqtt_pub_start_cb: topic %s\n", topic);

    if (tot_len > 1024)
    {
        DEBUG_printf("Message length exceeds buffer size, discarding");
    }
    else
    {
        data_in = tot_len;
        data_len = 0;
    }
}

static void mqtt_pub_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    if (data_in > 0)
    {
        data_in -= len;
        memcpy(&buffer[data_len], data, len);
        data_len += len;

        if (data_in == 0)
        {
            buffer[data_len] = 0;
            DEBUG_printf("Message received: %s\n", &buffer);
        }
    }
}

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

void mqtt_pub_request_cb(void *arg, err_t err)
{
    MQTT_CLIENT_T *state = (MQTT_CLIENT_T *)arg;
    DEBUG_printf("mqtt_pub_request_cb: err %d\n", err);
    state->received++;
}

void mqtt_sub_request_cb(void *arg, err_t err)
{
    DEBUG_printf("mqtt_sub_request_cb: err %d\n", err);
}

err_t mqtt_test_publish(MQTT_CLIENT_T *state)
{
    char buffer[128];

    sprintf(buffer, "{\"message\":\"hello from picow %d / %d\"}", state->received, state->counter);

    err_t err;
    u8_t qos = 0; /* 0 1 or 2, see MQTT specification.  AWS IoT does not support QoS 2 */
    u8_t retain = 0;
    cyw43_arch_lwip_begin();
    err = mqtt_publish(state->mqtt_client, "pico_w/test", buffer, strlen(buffer), qos, retain, mqtt_pub_request_cb, state);
    cyw43_arch_lwip_end();
    if (err != ERR_OK)
    {
        DEBUG_printf("Publish err: %d\n", err);
    }

    return err;
}

err_t mqtt_test_connect(MQTT_CLIENT_T *state)
{
    struct mqtt_connect_client_info_t ci;
    err_t err;

    memset(&ci, 0, sizeof(ci));

    ci.client_id = "PicoW";
    ci.client_user = MQTT_USERNAME;
    ci.client_pass = MQTT_PASSWORD;
    ci.keep_alive = 0;
    ci.will_topic = NULL;
    ci.will_msg = NULL;
    ci.will_retain = 0;
    ci.will_qos = 0;

    const struct mqtt_connect_client_info_t *client_info = &ci;

    err = mqtt_client_connect(state->mqtt_client, &(state->remote_addr), MQTT_SERVER_PORT, mqtt_connection_cb, state, client_info);

    if (err != ERR_OK)
    {
        DEBUG_printf("mqtt_connect return %d\n", err);
    }

    return err;
}

void printIPv4Address(unsigned int addr)
{
    // Extracting individual octets
    unsigned char octet4 = (addr >> 24) & 0xFF;
    unsigned char octet3 = (addr >> 16) & 0xFF;
    unsigned char octet2 = (addr >> 8) & 0xFF;
    unsigned char octet1 = addr & 0xFF;

    // Printing the IPv4 address
    printf("%u.%u.%u.%u\n", octet1, octet2, octet3, octet4);
}

void mqtt_run_test(MQTT_CLIENT_T *state)
{
    state->mqtt_client = mqtt_client_new();

    state->counter = 0;

    if (state->mqtt_client == NULL)
    {
        DEBUG_printf("Failed to create new mqtt client\n");
        return;
    }
    // psa_crypto_init();
    if (mqtt_test_connect(state) == ERR_OK)
    {
        absolute_time_t timeout = nil_time;
        bool subscribed = false;
        mqtt_set_inpub_callback(state->mqtt_client, mqtt_pub_start_cb, mqtt_pub_data_cb, 0);

        while (true)
        {
            cyw43_arch_poll();
            absolute_time_t now = get_absolute_time();
            if (is_nil_time(timeout) || absolute_time_diff_us(now, timeout) <= 0)
            {
                if (mqtt_client_is_connected(state->mqtt_client))
                {
                    cyw43_arch_lwip_begin();

                    if (!subscribed)
                    {
                        mqtt_sub_unsub(state->mqtt_client, "pico_w/recv", 0, mqtt_sub_request_cb, 0, 1);
                        subscribed = true;
                    }

                    if (mqtt_test_publish(state) == ERR_OK)
                    {
                        if (state->counter != 0)
                        {
                            DEBUG_printf("published %d\n", state->counter);
                        }
                        timeout = make_timeout_time_ms(5000);
                        state->counter++;
                    } // else ringbuffer is full and we need to wait for messages to flush.
                    cyw43_arch_lwip_end();
                }
                else
                {
                    // DEBUG_printf(".");
                }
            }
        }
    }
}

int main()
{
    stdio_init_all();
    printf("Build Version: 101. Press Resume on debugger to continue.\n");

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
    MQTT_CLIENT_T *mqtt_state = mqtt_client_init();
    run_dns_lookup(mqtt_state);
    mqtt_run_test(mqtt_state);
    cyw43_arch_deinit();
    printf("Bye!\n");
    return 0;
}
