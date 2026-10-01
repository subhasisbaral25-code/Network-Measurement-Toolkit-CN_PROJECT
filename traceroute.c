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
} __attribute__((packed,aligned(2)));// Ensure no padding is added by the compiler

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

//Run Traceroute in ICMP Mode
void run_icmp_traceroute(const char *target_ip_str, int max_hops) {
   
int sockfd=socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
if (sockfd<0){
    perror("ICMP Socket creation failed (Are you running with sudo?)");
    return;
}

struct sockaddr_in target_ip;//It is designed to hold an IPv4 address and port no.
memset(&target_ip,0,sizeof(target_ip));//memset completely fills the target_ip structure with zeros.Thus preventing from garbage value.
target_ip.sin_family=AF_INET;
if(inet_pton(AF_INET,target_ip_str,&target_ip.sin_addr)<=0){
    printf("Error: Invalid target IP address '%s'\n",target_ip_str);
    close(sockfd);
    return;
}

struct timeval timeout ={2,0};
// Configure the socket to stop waiting after 2 seconds so it doesn't freeze if a router drops the packet
if(setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))<0){
  perror("Failed to set receive timeout");
  close(sockfd);
  return;
}

int packets_sent=0;
int packets_received=0;
//acts as a flag
int destination_reached=0;

printf("\n===== Starting ICMP Traceroute to %s =====\n",target_ip_str);

// TTL(time to live ) which is increased by 1 for each hop
for(int ttl=1;ttl<=max_hops;ttl++){
    // Inject the current TTL limit directly into the IP packet header
    if(setsockopt(sockfd,IPPROTO_IP,IP_TTL,&ttl,sizeof(ttl))<0){
        perror("Failed to set IP_TTL option");
        break;
    }
//Builds Ping part

struct icmp_header icmp_packet;
icmp_packet.type=8;//8 = ICMP Echo Request
icmp_packet.code=0;//0 = Standard code for Echo Request
// Stamp the packet with our Process ID and current Hop number (TTL) 
icmp_packet.identifier=htons(getpid());
icmp_packet.sequence=htons(ttl);
icmp_packet.checksum=0;
icmp_packet.checksum=calculate_checksum((uint16_t *)&icmp_packet,sizeof(icmp_packet));// Calculate the mathematical checksum 

struct timeval start_time,end_time;//stopwatch declared
gettimeofday(&start_time, NULL);// stopwatch started

// Fire the ICMP packet out into the network toward the target IP
ssize_t bytes_sent = sendto(sockfd, &icmp_packet, sizeof(icmp_packet), 0, (struct sockaddr *)&target_ip, sizeof(target_ip));

// Check if the operating system failed to send the packet
if (bytes_sent <= 0) {
    printf("TTL = %d | Packet launch failed\n", ttl);
    continue;
}
packets_sent++;

// Flag to track whether we get a reply from this specific router
int hop_responded = 0;

// Continuously listen for packets until we catch our specific reply or the 2-second timer runs out
while(1){
    char recv_buffer[1024];
    struct sockaddr_in router_ip;
    socklen_t router_ip_len = sizeof(router_ip);
    // Wait to catch an incoming network packet
    ssize_t bytes_received = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&router_ip, &router_ip_len);
    gettimeofday(&end_time, NULL);// stopwatch stopped
    if (bytes_received<=0) break;
    if (bytes_received < 20) continue;
    if ((recv_buffer[0] >> 4) != 4) continue;
    int outer_ip_len = (recv_buffer[0] & 0x0F) * 4;
    if (outer_ip_len < 20 || bytes_received < outer_ip_len + 8) continue;
    
    // Extract the ICMP data from the raw packet
    struct icmp_header *outer_icmp = (struct icmp_header *)(recv_buffer + outer_ip_len);
   
    double time_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) + ((end_time.tv_usec - start_time.tv_usec) / 1000.0);
     
    if (outer_icmp->type == 11 && outer_icmp->code == 0) {// 11 = ICMP_TIME_EXCEEDED
        int inner_ip_offset = outer_ip_len + 8;
        if (bytes_received < inner_ip_offset + 20) continue;
        if ((recv_buffer[inner_ip_offset] >> 4) != 4) continue;
        int inner_ip_len = (recv_buffer[inner_ip_offset] & 0x0F) * 4;
        if (inner_ip_len < 20) continue;
        
        if (bytes_received < inner_ip_offset + inner_ip_len + 8) continue;

        struct icmp_header *inner_icmp = (struct icmp_header *)(recv_buffer+inner_ip_offset+inner_ip_len);
    // Verify this is OUR packet: check the Type, Process ID, and current Hop sequence
    // (ntohs converts the network byte order back to a standard integer)
    if (inner_icmp->type == ICMP_ECHO_REQUEST && ntohs(inner_icmp->identifier) == (uint16_t)getpid() && 
        ntohs(inner_icmp->sequence) == (uint16_t)ttl) {
        packets_received++;
        printf("%2d  %s  %.2f ms (TTL Expired)\n", ttl, inet_ntoa(router_ip.sin_addr), time_ms);
        hop_responded = 1;
        break;
}
}
   // If Echo Reply == Type 0 then hit the final target 
   else if (outer_icmp->type == 0 && outer_icmp->code == 0) { 

    // Make sure this reply belongs to our specific ping
    if (ntohs(outer_icmp->identifier) == (uint16_t)getpid() && ntohs(outer_icmp->sequence) == (uint16_t)ttl) {
                    
    // Count the packets and print the final IP and time
        packets_received++;
        printf("%2d  %s  %.2f ms Destination Reached\n", ttl, inet_ntoa(router_ip.sin_addr), time_ms);
                    
        // It is set to 1 so the entire program knows to stop looping
        destination_reached = 1;
        hop_responded = 1;
        break;
    }

}  
}
 if (!hop_responded) printf("TTL = %2d | Request timed out\n", ttl);
        if (destination_reached) break;
        usleep(200000); // 200ms delay between hops
}
printf("\n--- Traceroute Statistics ---\n");
    printf("%d packets sent, %d packets received, %.1f%% packet loss\n", 
           packets_sent, packets_received, 
           packets_sent > 0 ? ((float)(packets_sent - packets_received) / packets_sent) * 100.0 : 0.0);

    close(sockfd);
}

// Run Traceroute in UDP Mode
void run_udp_traceroute(const char *target_ip_str, int max_hops) {
   int send_sockfd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
   int recv_sockfd=socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);

   if(send_sockfd<0 || recv_sockfd<0){
       perror("Socket creation failed (Are you running with sudo ?)");
       if(send_sockfd>=0) close(send_sockfd);
       if(recv_sockfd>=0) close(recv_sockfd);
       return;
   }

struct sockaddr_in target_ip;//It is designed to hold an IPv4 address and port no.
memset(&target_ip,0,sizeof(target_ip));//memset completely fills the target_ip structure with zeros.Thus preventing from garbage value.
target_ip.sin_family=AF_INET;

if(inet_pton(AF_INET,target_ip_str,&target_ip.sin_addr)<=0){
    printf("Error: Invalid target IP address '%s'\n",target_ip_str);
    close(send_sockfd);
    close(recv_sockfd);
    return;
}

struct timeval timeout ={2,0};
// Configure the socket to stop waiting after 2 seconds so it doesn't freeze if a router drops the packet
if(setsockopt(recv_sockfd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))<0){
  perror("Failed to set receive timeout");
  close(send_sockfd);
  close(recv_sockfd);
  return;
}

int packets_sent=0;
int packets_received=0;
//acts as a flag
int destination_reached=0;

printf("\n===== Starting UDP Traceroute to %s =====\n",target_ip_str);

// TTL(time to live ) which is increased by 1 for each hop
for(int ttl=1;ttl<=max_hops;ttl++){
// Inject the current TTL limit directly into the IP packet header
 if(setsockopt(send_sockfd,IPPROTO_IP,IP_TTL,&ttl,sizeof(ttl))<0){
    perror("Failed to set IP_TTL option");
    break;
    }
// Increment the destination port based on the current TTL
// This helps uniquely identify responses and ensures we hit an unused port at the destination    
uint16_t target_port=UDP_BASE_PORT+ttl;
target_ip.sin_port=htons(target_port);

// Small text payload sent with the UDP packet to trigger router responses
//An empty payload looks suspicious to the network so I have put "CN_PROJECT_TRACE"
char payload[]="CN_PROJECT_TRACE";

struct timeval start_time,end_time;//stopwatch declared
gettimeofday(&start_time,NULL);//stopwatch started

// Send the UDP probe packet to the target IP address
ssize_t bytes_sent=sendto(send_sockfd,payload,strlen(payload),0,(struct sockaddr *)&target_ip,sizeof(target_ip));

if(bytes_sent<=0){
    printf("TTL = %d | Sending of UDP Packet FAILED.\n",ttl);
    continue;
}
packets_sent++;

// A flag to track whether a router actually replies to this specific probe
int hop_responded=0;

while(1){
    char recv_buffer[1024];
    struct sockaddr_in router_ip;
    socklen_t router_ip_len=sizeof(router_ip);
    // Wait to catch an incoming network packet
    ssize_t bytes_received = recvfrom(recv_sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&router_ip, &router_ip_len);
    gettimeofday(&end_time, NULL);// stopwatch stopped
    if (bytes_received<=0) break;
    if (bytes_received < 20) continue;
    if ((recv_buffer[0] >> 4) != 4) continue;
    int outer_ip_len = (recv_buffer[0] & 0x0F) * 4;
    if (outer_ip_len < 20 || bytes_received < outer_ip_len + 8) continue;
    
    // Extract the ICMP data from the raw packet
    struct icmp_header *outer_icmp = (struct icmp_header *)(recv_buffer + outer_ip_len);
   
    double time_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) + ((end_time.tv_usec - start_time.tv_usec) / 1000.0);
    //11 = ICMP_TIME_EXCEEDED , 3 = ICMP_DEST_UNREACH , 3 = ICMP_PORT_UNREACH 
    if((outer_icmp->type == 11 && outer_icmp->code == 0) || (outer_icmp->type == 3 && outer_icmp->code == 3)){

        int inner_ip_offset = outer_ip_len + 8;
        if(bytes_received<inner_ip_offset + 20)continue;
        if ((recv_buffer[inner_ip_offset] >> 4) != 4) continue;
        int inner_ip_len = (recv_buffer[inner_ip_offset] & 0x0F) * 4;
        if (inner_ip_len < 20) continue;
       
        uint8_t inner_protocol = (uint8_t)recv_buffer[inner_ip_offset + 9];
        if (inner_protocol != IPPROTO_UDP) continue;

        if (bytes_received < inner_ip_offset + inner_ip_len + 4) continue; 
        
        // Extract the destination port from the returned UDP header
        uint16_t inner_dest_port;
        memcpy(&inner_dest_port, recv_buffer + inner_ip_offset + inner_ip_len + 2, sizeof(inner_dest_port));
        inner_dest_port = ntohs(inner_dest_port);

        if(inner_dest_port == target_port){
            packets_received++;
            if(outer_icmp->type == 11){//11 = ICMP_TIME_EXCEEDED
               printf("%2d %s %.2f ms | TTL Expired\n",ttl,inet_ntoa(router_ip.sin_addr),time_ms); 
            }
            else{
                printf("%2d %s %.2f ms | Destination Reached (Port Unreachable)\n",ttl,inet_ntoa(router_ip.sin_addr),time_ms); 
                destination_reached=1;// It is set to 1 so the entire program knows to stop looping
            
            }
            hop_responded=1;
            break;
        }
    }
     
}
if (!hop_responded) printf("TTL = %2d | Request timed out\n", ttl);
if (destination_reached) break;
usleep(200000);// 200ms delay between hops
}
printf("\n--- Traceroute Statistics ---\n");
    printf("%d packets sent, %d packets received, %.1f%% packet loss\n", 
    packets_sent, packets_received, 
    packets_sent > 0 ? ((float)(packets_sent - packets_received) / packets_sent) * 100.0 : 0.0);

close(send_sockfd);
close(recv_sockfd);
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