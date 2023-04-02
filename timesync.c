#include <pico/stdlib.h>
#include <hardware/rtc.h>
#include <stdio.h>
#include <stdbool.h>
#include <FreeRTOS.h>
#include <task.h>
#include <lwip/opt.h>
#include <lwip/arch.h>
#include <lwip/api.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>

#include "timesync.h"

#define TIMESYNC_TASK_PRIORITY  6
static TaskHandle_t timesync_task_handle = NULL;

static void timesync_task(void *params);

#define NTP_TIMESTAMP_DELTA 2208988800ull

#define NTP_SERVER "pool.ntp.org"
#define NTP_PORT 123

#define TIMEZONE (2 * 60 * 60)

#define SYNC_PERIOD (60 * 60 * configTICK_RATE_HZ)

static const struct tm START_TIME = {
    .tm_year = 2023,
    .tm_mon = 4,
    .tm_mday = 2,
    .tm_wday = 0,
    .tm_hour = 13,
    .tm_min = 30,
    .tm_sec = 0,
};

extern long timezone;
extern int daylight;

typedef struct {
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint32_t ref_timestamp_secs;
    uint32_t ref_timestamp_frac;
    uint32_t orig_timestamp_secs;
    uint32_t orig_timestamp_frac;
    uint32_t rx_timestamp_secs;
    uint32_t rx_timestamp_frac;
    uint32_t tx_timestamp_secs;
    uint32_t tx_timestamp_frac;
} ntp_packet;


void timesync_init(void)
{
    xTaskCreate(timesync_task,
                "timesync",
                1024,
                NULL,
                TIMESYNC_TASK_PRIORITY,
                &timesync_task_handle);
}

static void sync_rtc(const struct tm *sys_tm)
{
    datetime_t dt;

    dt.year  = sys_tm->tm_year;
    dt.month = sys_tm->tm_mon + 1;
    dt.day   = sys_tm->tm_mday;
    dt.dotw  = sys_tm->tm_wday;
    dt.hour  = sys_tm->tm_hour;
    dt.min   = sys_tm->tm_min;
    dt.sec   = sys_tm->tm_sec;

    rtc_set_datetime(&dt);
}

static bool sync_time(void)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if (sockfd < 0) {
        printf("Error: socket\n");
        return false;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(NTP_PORT);

    struct hostent *host = gethostbyname(NTP_SERVER);
    if (host == NULL)
    {
        printf("Error: gethostbyname\n");
        close(sockfd);
        return false;
    }

    memcpy(&server_addr.sin_addr, host->h_addr_list[0], host->h_length);

    ntp_packet packet;
    memset(&packet, 0, sizeof(packet));
    packet.li_vn_mode = (0 << 6) | (3 << 3) | 3;

    if (sendto(sockfd, &packet, sizeof(packet), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Error: sendto\n");
        close(sockfd);
        return false;
    }

    socklen_t server_addr_len = sizeof(server_addr);
    
    if (recvfrom(sockfd, &packet, sizeof(packet), 0,
                 (struct sockaddr *)&server_addr, &server_addr_len) < 0)
    {
        printf("Error: recvfrom\n");
        close(sockfd);
        return false;
    }

    uint32_t ntp_secs = ntohl(packet.rx_timestamp_secs);
    time_t current_time = ntp_secs - NTP_TIMESTAMP_DELTA + TIMEZONE;
    struct tm *sys_tm = gmtime(&current_time);

    sync_rtc(sys_tm);

    close(sockfd);

    return true;
}


static void timesync_task(void *params)
{
    static uint32_t last_sync = 0;
    struct netconn *conn;

    netconn_thread_init();

    sync_rtc(&START_TIME);

    for (;;)
    {
        if (last_sync == 0 || (xTaskGetTickCount() - last_sync) >= SYNC_PERIOD)
        {
            if (sync_time())
            {
                printf("RTC succesfully synced with pool.ntp.org\n");
                last_sync = xTaskGetTickCount();
            } 
        }

        vTaskDelay(5000);
    }
}
