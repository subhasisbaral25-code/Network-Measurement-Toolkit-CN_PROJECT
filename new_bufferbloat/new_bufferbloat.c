#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

// 1. DIRECTLY INCLUDE DIPTALAL'S PMTU MODULE (pmtu.h & pmtu.c)
#include "pmtu.h"
#define main diptalal_pmtu_main
#include "pmtu.c"
#undef main

// 2. DIRECTLY INCLUDE RUPEN'S ORIGINAL PING MODULE (ping.c)
#define calculate_checksum rupen_calculate_checksum
#define main rupen_ping_standalone_main
#include "ping.c"
#undef main
#undef calculate_checksum

// 3. DIRECTLY INCLUDE RUPEN'S UPDATED PING MODULE (updated_ping.c)
#define icmp_header updated_icmp_header
#define calculate_checksum updated_rupen_calculate_checksum
#define main updated_rupen_ping_standalone_main
#include "updated_ping.c"
#undef main
#undef calculate_checksum
#undef icmp_header

// 4. DIRECTLY INCLUDE DIPTALAL'S BANDWIDTH MODULE (bandwidth.c)
#define read_be16 bw_read_be16
#define calculate_checksum bw_calculate_checksum
#define main diptalal_bandwidth_main
#include "bandwidth.c"
#undef main
#undef calculate_checksum
#undef read_be16

// 5. DIRECTLY INCLUDE SUBHASIS'S TRACEROUTE MODULE (traceroute.c)
#define icmp_header trace_icmp_header
#define calculate_checksum subhasis_calculate_checksum
#define main subhasis_traceroute_main
#include "traceroute.c"
#undef main
#undef calculate_checksum
#undef icmp_header

// 6. RAW-SOCKET RTT HELPER (Using Rupen's Ping Engine)
double measure_rtt_via_rupen_ping(const char *target_ip_str, int seq_offset, const char *phase_label) {
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("Socket creation failed!!!");
        return -1.0;
    }

    struct sockaddr_in target_ip;
    memset(&target_ip, 0, sizeof(target_ip));
    target_ip.sin_family = AF_INET;
    if (inet_pton(AF_INET, target_ip_str, &target_ip.sin_addr) <= 0) {
        close(sockfd);
        return -1.0;
    }

    struct timeval timeout = {0, 300000}; // 300ms slice
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int packets_received = 0;
    double total_rtt = 0.0;

    printf("\n--- [%s] Measuring RTT via Rupen's Ping Engine to %s ---\n", phase_label, target_ip_str);

    for (int i = 0; i < 4; i++) {
        // Drain any old ICMP packets before sending
        char flush_buf[1024];
        while (recvfrom(sockfd, flush_buf, sizeof(flush_buf), MSG_DONTWAIT, NULL, NULL) > 0);

        struct icmp_header icmp_packet;
        icmp_packet.type = 8;
        icmp_packet.code = 0;
        icmp_packet.identifier = (uint16_t)(getpid() & 0xFFFF);
        icmp_packet.sequence = seq_offset + i + 1;
        icmp_packet.checksum = 0;
        uint16_t csum_buf[sizeof(icmp_packet) / 2 + 1]; memcpy(csum_buf, &icmp_packet, sizeof(icmp_packet)); icmp_packet.checksum = rupen_calculate_checksum(csum_buf, sizeof(icmp_packet));

        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);

        sendto(sockfd, &icmp_packet, sizeof(icmp_packet), 0, (struct sockaddr *)&target_ip, sizeof(target_ip));

        while (1) {
            char recv_buffer[1024];
            struct sockaddr_in router_ip;
            socklen_t router_ip_len = sizeof(router_ip);

            ssize_t bytes_received = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0,
                                              (struct sockaddr *)&router_ip, &router_ip_len);
            gettimeofday(&end_time, NULL);

            double elapsed_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) +
                                ((end_time.tv_usec - start_time.tv_usec) / 1000.0);

            if (bytes_received >= 28) {
                int ip_hdr_len = (recv_buffer[0] & 0x0F) * 4;
                struct icmp_header *received_icmp = (struct icmp_header *)(recv_buffer + ip_hdr_len);

                if (received_icmp->type == 0 &&
                    received_icmp->identifier == icmp_packet.identifier &&
                    received_icmp->sequence == icmp_packet.sequence) {
                    total_rtt += elapsed_ms;
                    packets_received++;
                    printf("Reply from %s: icmp_seq=%d RTT=%.2f ms\n",
                           target_ip_str, received_icmp->sequence, elapsed_ms);
                    break;
                }
            }

            if (elapsed_ms >= 2000.0) {
                printf("Timeout waiting for icmp_seq=%d (elapsed %.0f ms)\n", icmp_packet.sequence, elapsed_ms);
                break;
            }
        }
        usleep(250000);
    }

    close(sockfd);
    return (packets_received > 0) ? (total_rtt / packets_received) : -1.0;
}

// 7. AMLAN'S BUFFERBLOAT UDP LINK SATURATION ENGINE
volatile int keep_saturating = 0;
int saturation_payload_size = 1400;

void *saturate_link_worker(void *arg) {
    const char *target_ip_str = (const char *)arg;
    int udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_sock < 0) return NULL;

    // Cross-traffic sink: saturate the bottleneck uplink router using an alternate sink IP
    // so the target IP itself doesn't drop/rate-limit our ICMP Echo Requests
    const char *sink_ip = (strcmp(target_ip_str, "1.1.1.1") == 0) ? "8.8.8.8" : "1.1.1.1";

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(443); // High-priority HTTPS/QUIC UDP port
    inet_pton(AF_INET, sink_ip, &dest.sin_addr);

    char payload[1400];
    memset(payload, 'A', sizeof(payload));

    while (keep_saturating) {
        for (int b = 0; b < 8; b++) {
            sendto(udp_sock, payload, saturation_payload_size, 0, (struct sockaddr *)&dest, sizeof(dest));
        }
        usleep(1000);
    }

    close(udp_sock);
    return NULL;
}

// 8. UNIFIED MAIN ORCHESTRATOR
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: sudo %s <Target IP Address>\n", argv[0]);
        return -1;
    }

    const char *target_ip = argv[1];

    printf("\n================ [1] RUPEN'S ORIGINAL PING (ping.c) ================\n");
    char *ping_args[] = {"ping", (char *)target_ip, NULL};
    rupen_ping_standalone_main(2, ping_args);

    printf("\n================ [2] RUPEN'S UPDATED PING (updated_ping.c) ================\n");
    char *updated_ping_args[] = {"updated_ping", (char *)target_ip, NULL};
    updated_rupen_ping_standalone_main(2, updated_ping_args);

    printf("\n================ [3] SUBHASIS'S TRACEROUTE (traceroute.c) ================\n");
    char *trace_icmp_args[] = {"traceroute", (char *)target_ip, "-i", NULL};
    subhasis_traceroute_main(3, trace_icmp_args);

    char *trace_udp_args[] = {"traceroute", (char *)target_ip, "-u", NULL};
    subhasis_traceroute_main(3, trace_udp_args);

    printf("\n================ [4] DIPTALAL'S PATH MTU DISCOVERY (pmtu.c) ================\n");
    discover_pmtu(target_ip);

    printf("\n================ [5] DIPTALAL'S BANDWIDTH ESTIMATION (bandwidth.c) ================\n");
    char *bw_args[] = {"bandwidth", (char *)target_ip, NULL};
    diptalal_bandwidth_main(2, bw_args);

    printf("\n================ [6] AMLAN'S BUFFERBLOAT ENGINE ================\n");
    double idle_rtt = measure_rtt_via_rupen_ping(target_ip, 0, "IDLE_PHASE");

    keep_saturating = 1;
    pthread_t load_thread;
    pthread_create(&load_thread, NULL, saturate_link_worker, (void *)target_ip);

    usleep(200000);
    double loaded_rtt = measure_rtt_via_rupen_ping(target_ip, 100, "LOADED_PHASE");

    keep_saturating = 0;
    pthread_join(load_thread, NULL);

    if (idle_rtt < 0) idle_rtt = 0.0;
    if (loaded_rtt < 0) loaded_rtt = 0.0;

    double delta = (loaded_rtt > idle_rtt) ? (loaded_rtt - idle_rtt) : 0.0;
    printf("\nBufferbloat Summary for %s -> Idle RTT: %.2f ms | Loaded RTT: %.2f ms | Queuing Delay: %.2f ms\n",
           target_ip, idle_rtt, loaded_rtt, delta);

    printf("\nRESULT_CSV:%s,%.2f,%.2f\n", target_ip, idle_rtt, loaded_rtt);

    return 0;
}
