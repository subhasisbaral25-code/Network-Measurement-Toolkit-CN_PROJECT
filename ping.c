#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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

int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);//we request raw socket from linux kernel
//SOCK_RAW : we build our own headers , IPPROTO_ICMP: ping protocol

if(sockfd <0){
    perror("Socket creation failed!!!");
    return -1;
}
printf("Raw socket successfully opened. File descriptor: %d\n",sockfd);

struct icmp_header icmp_packet;

icmp_packet.type = 8;              // 8 = ICMP Echo Request
    icmp_packet.code = 0;              // 0 = Standard code for Echo Request
    icmp_packet.identifier = getpid(); // Use the Linux Process ID as our unique tag
    icmp_packet.sequence = 1;          // This is packet #1
    icmp_packet.checksum = 0;          // Must be 0 before calculation

    //generate RFC1071 checksum value
icmp_packet.checksum = calculate_checksum((uint16_t *)&icmp_packet, sizeof(icmp_packet));
//pass the memory addr of aour pavket(&icmp_packet) cast to 16 bit pointer

    printf("ICMP Echo Request packet constructed! Checksum generated: 0x%04x\n", icmp_packet.checksum);


close(sockfd);

    return 0;
}