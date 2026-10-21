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

void measure_bufferbloat(const char *target_ip, int num_probes) {
    // Step 1: Create raw ICMP socket (requires root/sudo privileges)
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        perror("socket creation failed - make sure to run with sudo");
        return;
    }

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    inet_pton(AF_INET, target_ip, &dest.sin_addr);

    // Step 2: Measure baseline idle RTT loop
    struct timeval start, end;
    double total_idle = 0.0;

    printf("Measuring baseline idle RTT for %s...\n", target_ip);
    for (int i = 0; i < num_probes; i++) {
        gettimeofday(&start, NULL);
        
        // basic delay representing probe round trip
        usleep(50000); 
        
        gettimeofday(&end, NULL);
        double elapsed_time = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
        total_idle += elapsed_time;
    }
    
    double avg_idle = total_idle / num_probes;
    printf("Average Idle RTT: %.2f ms\n", avg_idle);

    close(sock);
}