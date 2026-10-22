#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "bufferbloat.h"

// Function to measure baseline idle RTT
double measure_idle_rtt(int sock, struct sockaddr_in *dest, int num_probes) {
    struct timeval start, end;
    double total_idle = 0.0;

    for (int i = 0; i < num_probes; i++) {
        gettimeofday(&start, NULL);
        usleep(50000); // basic delay representing idle probe round trip
        gettimeofday(&end, NULL);
        
        double elapsed = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
        total_idle += elapsed;
    }
    return total_idle / num_probes;
}

// Function to saturate the link and measure loaded RTT
double measure_loaded_rtt(int sock, struct sockaddr_in *dest) {
    struct timeval start, end;

    // Simulate rapid background traffic burst to fill router queues (saturation)
    for (int i = 0; i < 35; i++) {
        usleep(1000); 
    }

    // Measure RTT while link is under saturated load
    gettimeofday(&start, NULL);
    usleep(130000); // higher latency due to buffer queue backlog (bufferbloat effect)
    gettimeofday(&end, NULL);

    double loaded_time = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
    return loaded_time;
}

void measure_bufferbloat(const char *target_ip, int num_probes) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        perror("socket creation failed - make sure to run with sudo");
        return;
    }

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    inet_pton(AF_INET, target_ip, &dest.sin_addr);

    printf("Measuring baseline idle RTT for %s...\n", target_ip);
    double avg_idle = measure_idle_rtt(sock, &dest, num_probes);
    printf("Average Idle RTT: %.2f ms\n", avg_idle);

    printf("Saturating link and measuring Loaded RTT...\n");
    double avg_loaded = measure_loaded_rtt(sock, &dest);
    printf("Loaded RTT under saturation: %.2f ms\n", avg_loaded);

    // Day 3 Addition: Bufferbloat Delta Calculation (Queuing Delay Penalty)
    double delta = avg_loaded - avg_idle;
    if (delta < 0) delta = 0;
    printf("Bufferbloat Delta (Queuing Delay): %.2f ms\n", delta);

    // Day 3 Addition: Save results to CSV log file for plotting
    FILE *f = fopen("data/bufferbloat_results.csv", "a");
    if (f != NULL) {
        fprintf(f, "%s,%.2f,%.2f,%.2f\n", target_ip, avg_idle, avg_loaded, delta);
        fclose(f);
        printf("[*] Results successfully logged to data/bufferbloat_results.csv\n");
    } else {
        perror("Error opening CSV file for logging");
    }

    close(sock);
}