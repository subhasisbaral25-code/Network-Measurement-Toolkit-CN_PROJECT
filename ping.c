#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct icmp_header {
    uint8_t type;        // ICMP message type 1byte
    uint8_t code;        // ICMP message code 1byte
    uint16_t checksum;   // Checksum 2bytes
    uint16_t identifier; // Identifier to match requests and replies 2B
    uint16_t sequence;   // Sequence number to track requests 2B
} __attribute__((packed)); // Ensure no padding is added by the compiler

int main() {
    printf("Network core initialized.Memory structure defined.\n");
return 0;
}