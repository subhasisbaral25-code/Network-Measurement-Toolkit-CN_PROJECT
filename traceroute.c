#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#define ICMP_ECHO_REQUEST 8
#define ICMP_TIME_EXCEEDED 11
#define ICMP_DEST_UNREACH 3
#define ICMP_PORT_UNREACH 3
#define UDP_BASE_PORT 33434

struct icmp_header {
    uint8_t type;         // ICMP message type 1 byte
    uint8_t code;         // ICMP message code 1 byte
    uint16_t checksum;    // Checksum 2 bytes
    uint16_t identifier;  // Identifier to match requests and replies 2B
    uint16_t sequence;    // Sequence number to track requests 2B
} __attribute__((packed));// Ensure no padding is added by the compiler

// RFC 1071 checksum logic
uint16_t calculate_checksum(uint16_t *ptr, int nbytes) {
    uint32_t sum = 0;
    while (nbytes > 1) {
        sum += *ptr;
        ptr++; //here we jump exactly 2B 
        nbytes -= 2;
    }
    if (nbytes == 1) {
        uint8_t last_byte = *(uint8_t *)ptr;// temorarily treat pointer as 8bit
        sum += last_byte;//c compiler itself pads with 8 zeroes
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum = sum + (sum >> 16);
    return (uint16_t)~sum;
}

//These are the functions which will be filled in next commits.
void run_icmp_traceroute(const char *target_ip_str, int max_hops) {
    printf("ICMP Traceroute mode selected (Implementation pending)\n");
}

void run_udp_traceroute(const char *target_ip_str, int max_hops) {
    printf("UDP Traceroute mode selected (Implementation pending)\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: sudo %s <Target IP Address> <-i|-u>\n", argv[0]);
        printf("  -i : ICMP Traceroute mode\n");
        printf("  -u : UDP Traceroute mode\n");
        return -1;
    }

    if (strcmp(argv[2], "-i") == 0) {
        run_icmp_traceroute(argv[1], 30);
    } else if (strcmp(argv[2], "-u") == 0) {
        run_udp_traceroute(argv[1], 30);
    } else {
        printf("Invalid mode selection! Use -i for ICMP or -u for UDP.\n");
        return -1;
    }

    return 0;
}