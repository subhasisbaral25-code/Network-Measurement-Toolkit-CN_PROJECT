#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/time.h>

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
    void *packet_ptr = &icmp_packet; 
icmp_packet.checksum = calculate_checksum((uint16_t *)packet_ptr, sizeof(icmp_packet));
//pass the memory addr of aour pavket(&icmp_packet) cast to 16 bit pointer

    printf("ICMP Echo Request packet constructed! Checksum generated: 0x%04x\n", icmp_packet.checksum);

struct sockaddr_in target_ip;
target_ip.sin_family = AF_INET;

if(inet_pton(AF_INET, "8.8.8.8", &target_ip.sin_addr) <= 0) { //1=success(valid IP conversion),0=string format invalid,-1=sys level error
    perror("Invalid IP address format!");
    return -1;
}
struct timeval start_time , end_time;//stopwatch declared

gettimeofday(&start_time, NULL);// stopwatch started

ssize_t bytes_sent = sendto(sockfd, &icmp_packet, sizeof(icmp_packet), 0, (struct sockaddr *)&target_ip, sizeof(target_ip));

if(bytes_sent <= 0){
    perror("Packet launch failed");
}else{
    printf("Packet successfully fires , %zd bytes sent to 8.8.8.8\n",bytes_sent);
}

struct timeval timeout; // we create a strict 2sec socket timeout
timeout.tv_sec = 2;
timeout.tv_usec = 0;

if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    perror("Failed to set socket receive timeout");
    return -1;
}

char recv_buffer[1024];
struct sockaddr_in router_ip;
socklen_t router_ip_len = sizeof(router_ip);

printf("Listening to router response for 2 seconds...\n");

ssize_t bytes_received = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&router_ip, &router_ip_len);

if(bytes_received <=0){
    printf("Request timed out / failed to receive.\n");
}else {
    gettimeofday(&end_time,NULL); //stop the stopwatch
    printf("Reply caught ! Received %zd bytes from the network.\n",bytes_received);

// now unpacking the reply we received
int ip_header_length = 20;
struct icmp_header *received_icmp = (struct icmp_header *)(recv_buffer + ip_header_length);//we shifft by 20B to land exactly on 1st B of icmp payload

if(received_icmp -> type == 0){// check if its echo reply
if(received_icmp -> identifier == getpid() ){//verufy process id matches our specific program
printf("Identity verified! , the router replied to exact same process.\n ");

double time_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) + ((end_time.tv_usec - start_time.tv_usec)/ 1000.0 );
printf("Reply from 8.8.8.8: bytes= %zd RTT= %.2f ms\n",bytes_received,time_ms);
}else{
    printf("Warning : caught a icmp packet, but the pid does not match our same one.\n");
}
}else{
    printf("Warning : caught aicmp packet, but its not an echo reply(Type %d).\n", received_icmp ->type);
}

}


close(sockfd);

    return 0;
}