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

//RFC 1071 checksum logic
uint16_t calculate_checksum(uint16_t *ptr , int nbytes) {

    uint32_t sum=0;
 
    while(nbytes >1){
        sum += *ptr;
        ptr++; //here we jump exactly 2B 
        nbytes -=2;
    }
    if (nbytes == 1) {
        uint8_t last_byte = *(uint8_t *)ptr;// temorarily treat pointer as 8bit
    sum += last_byte; //c compiler itself pads with 8 zeroes
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum = sum + (sum >> 16);

    return (uint16_t)~sum;
}

int main() {
    printf("Network core initialized.Memory structure defined.\n");
return 0;
}