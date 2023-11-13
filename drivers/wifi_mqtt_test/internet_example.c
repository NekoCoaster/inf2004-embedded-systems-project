// https://rfwumcu.blogspot.com/2023/02/raspberry-pi-pico-w-c-sdk-lwip-ep-4.html
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "pico/cyw43_arch.h"
#include "hardware/rtc.h"

#include "lwip/apps/mqtt.h"
#include "ws2812.pio.h"
#include "hardware/clocks.h"
#include "ntp_time.h"
#include "cJSON.h"

#define WIFI_SSID "your_SSID"
#define WIFI_PASSWORD "your_PASSWORD"

typedef struct MQTT_CLIENT_DATA_T_
{
    mqtt_client_t *mqtt_client_inst;
    struct mqtt_connect_client_info_t mqtt_client_info;
    uint8_t data[MQTT_OUTPUT_RINGBUF_SIZE];
    uint8_t topic[100];
    uint32_t len;
    bool playing;
    bool newTopic;
} MQTT_CLIENT_DATA_T;

MQTT_CLIENT_DATA_T *mqtt;

struct mqtt_connect_client_info_t mqtt_client_info =
    {
        "ws2812",
        NULL, /* user */
        NULL, /* pass */
        0,    /* keep alive */
        NULL, /* will_topic */
        NULL, /* will_msg */
        0,    /* will_qos */
        0     /* will_retain */
#if LWIP_ALTCP && LWIP_ALTCP_TLS
        ,
        NULL
#endif
};

#define NUM_PIXELS 60
#define WS2812_PIN 16

#define green 0xff0000
#define red 0x00ff00
#define blue 0x0000ff
uint32_t color_pixel[NUM_PIXELS];
repeating_timer_t colok_timer;

static inline void ws2812_program_init(PIO pio, uint sm, uint pin, float freq)
{
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);

    uint offset = pio_add_program(pio, &ws2812_program);

    pio_sm_config c = ws2812_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, pin);
    sm_config_set_out_shift(&c, false, true, 24);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    int cycles_per_bit = ws2812_T1 + ws2812_T2 * 2;
    float div = clock_get_hz(clk_sys) / (freq * cycles_per_bit);
    sm_config_set_clkdiv(&c, div);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) |
           ((uint32_t)(g) << 16) |
           (uint32_t)(b);
}

void wait_connecting()
{
    for (int i = 0; i < NUM_PIXELS; i++)
        put_pixel(red);
}

void clear_pixel()
{
    for (int i = 0; i < NUM_PIXELS; i++)
        put_pixel(0x000000);
}

void connected()
{
    for (int i = 0; i < NUM_PIXELS; i++)
        put_pixel(green);
}

void random_pixel(uint8_t color_index, uint len, int dir, uint32_t color, uint8_t speed)
{
    static uint t = 0;

    while (mqtt->playing)
    {
        if (len > NUM_PIXELS)
            len = NUM_PIXELS;
        for (int i = 0; i < NUM_PIXELS; i++)
            color_pixel[i] = 0x000000; // reset all pixel

        for (int i = t; i < (t + len); ++i)
        {
            switch (color_index)
            {
            case 1:
                color_pixel[(i) % NUM_PIXELS] = color;
                break;
            case 2:
                color_pixel[(i) % NUM_PIXELS] = (rand());
                break;
            }
        }

        for (int i = 0; i < NUM_PIXELS; i++)
            put_pixel(color_pixel[i]);
        t = (t + dir + NUM_PIXELS) % NUM_PIXELS;
        sleep_ms(10 * speed);
    }
    clear_pixel();
}

bool repeat_timer_cb(repeating_timer_t *rt)
{
    static datetime_t dt;

    rtc_get_datetime(&dt);

    for (int i = 0; i < NUM_PIXELS; i++)
        color_pixel[i] = 0x000000;
    color_pixel[(dt.hour % 12) * 5] |= green;
    color_pixel[dt.min] |= blue;
    color_pixel[dt.sec] |= red;

    for (int i = 0; i < NUM_PIXELS; i++)
        put_pixel(color_pixel[i]);

    return true;
}

void sparkle()
{

    while (mqtt->playing)
    {
        for (int i = 0; i < NUM_PIXELS; ++i)
            put_pixel(rand() % 32 ? 0 : 0xffffffff);
        sleep_ms(10);
    }
}

void ws2812_action()
{

    cancel_repeating_timer(&colok_timer);
    cJSON *json_obj = cJSON_CreateObject();
    json_obj = cJSON_Parse(mqtt->data);
    uint8_t *type = cJSON_GetStringValue(cJSON_GetObjectItem(json_obj, "type"));
    uint8_t speed = 11 - (uint8_t)cJSON_GetNumberValue(cJSON_GetObjectItem(json_obj, "speed"));
    uint8_t length = (uint8_t)cJSON_GetNumberValue(cJSON_GetObjectItem(json_obj, "length"));
    int8_t dir = (int8_t)cJSON_GetNumberValue(cJSON_GetObjectItem(json_obj, "dir"));
    uint8_t *colorstr = (cJSON_GetStringValue(cJSON_GetObjectItem(json_obj, "color")));
    uint32_t color = strtoul(colorstr, NULL, 16);

    color = (color & 0xff0000) >> 8 | (color & 0x00ff00) << 8 | color & 0x0000ff;

    if (strcmp(type, "clock") == 0)
    {
        mqtt->playing = true;
        add_repeating_timer_ms(-1000, repeat_timer_cb, NULL, &colok_timer);
    }
    if (strcmp(type, "sparkle") == 0)
    {
        mqtt->playing = true;
        sparkle();
    }
    if (strcmp(type, "randomcolor") == 0)
    {
        mqtt->playing = true;
        random_pixel(2, length, dir, 0x000000, speed); ////
    }
    if (strcmp(type, "pixelcolor") == 0)
    {
        mqtt->playing = true;
        random_pixel(1, length, dir, color, speed); ////
    }
    cJSON_Delete(json_obj);
}
void ws2812_stop()
{
    mqtt->playing = false;
}
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    MQTT_CLIENT_DATA_T *mqtt_client = (MQTT_CLIENT_DATA_T *)arg;
    LWIP_UNUSED_ARG(data);

    strncpy(mqtt_client->data, data, len);
    mqtt_client->len = len;
    mqtt_client->data[len] = '\0';

    mqtt_client->newTopic = true;
    mqtt->playing = false;
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
    MQTT_CLIENT_DATA_T *mqtt_client = (MQTT_CLIENT_DATA_T *)arg;
    strcpy(mqtt_client->topic, topic);
}

static void mqtt_request_cb(void *arg, err_t err)
{
    MQTT_CLIENT_DATA_T *mqtt_client = (MQTT_CLIENT_DATA_T *)arg;

    LWIP_PLATFORM_DIAG(("MQTT client \"%s\" request cb: err %d\n", mqtt_client->mqtt_client_info.client_id, (int)err));
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    MQTT_CLIENT_DATA_T *mqtt_client = (MQTT_CLIENT_DATA_T *)arg;
    LWIP_UNUSED_ARG(client);

    LWIP_PLATFORM_DIAG(("MQTT client \"%s\" connection cb: status %d\n", mqtt_client->mqtt_client_info.client_id, (int)status));

    if (status == MQTT_CONNECT_ACCEPTED)
    {
        mqtt_sub_unsub(client,
                       "start", 0,
                       mqtt_request_cb, arg,
                       1);
        mqtt_sub_unsub(client,
                       "stop", 0,
                       mqtt_request_cb, arg,
                       1);
    }
}

int main()
{
    stdio_init_all();
    PIO pio = pio0;
    int sm = 0;
    ws2812_program_init(pio, sm, WS2812_PIN, 800000);

    mqtt = (MQTT_CLIENT_DATA_T *)calloc(1, sizeof(MQTT_CLIENT_DATA_T));

    if (!mqtt)
    {
        printf("mqtt client instant ini error\n");
        return 0;
    }
    mqtt->playing = false;
    mqtt->newTopic = false;
    mqtt->mqtt_client_info = mqtt_client_info;

    if (cyw43_arch_init())
    {
        printf("failed to initialise\n");
        return 1;
    }
    wait_connecting();
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("failed to connect\n");
        return 1;
    }
    ntp_time_init();
    get_ntp_time();
    ip_addr_t addr;
    if (!ip4addr_aton("your_MQTT_SERVER_IP", &addr))
    {
        printf("ip error\n");
        return 0;
    }

    mqtt->mqtt_client_inst = mqtt_client_new();
    mqtt_set_inpub_callback(mqtt->mqtt_client_inst, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, mqtt);

    err_t err = mqtt_client_connect(mqtt->mqtt_client_inst, &addr, MQTT_PORT, &mqtt_connection_cb, mqtt, &mqtt->mqtt_client_info);
    if (err != ERR_OK)
    {
        printf("connect error\n");
        return 0;
    }
    connected();

    while (1)
    {
        if (mqtt->newTopic)
        {
            mqtt->newTopic = false;
            if (strcmp(mqtt->topic, "start") == 0)
            {
                ws2812_action();
            }
            if (strcmp(mqtt->topic, "stop") == 0)
            {
                ws2812_stop();
            }
        }
    }
    return 0;
}