/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "inf2004_credentials.h"

#define CYW43_HOST_NAME "INF2004 P9 PICO W"

#include <stdio.h>

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

int main()
{
    stdio_init_all();
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
    {
        printf("Connected to Wi-Fi\n");
    }
    while (true)
    {
        cyw43_arch_lwip_begin();
        printf("IP address: ");
        printIPv4Address(cyw43_state.netif[0].ip_addr.addr);
        cyw43_arch_lwip_end();
        sleep_ms(1000);
    }
    printf("Shutting down Wi-Fi module in 10 seconds...\n");
    sleep_ms(5000);
    printf("Wi-Fi module shut down\n");
    return 0;
}
