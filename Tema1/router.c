#include <queue.h>
#include "skel.h"
#include "router.h"


int main(int argc, char *argv[])
{
	setvbuf(stdout , NULL , _IONBF , 0);

	packet m;
	int rc;

	struct route_table_entry *rtable;
	struct arp_entry *arp_table = NULL;
	int arp_table_size = 0;
	
	queue q;
	q = queue_create();

	FILE *fp = fopen(argv[1], "r");
	if (!fp) {   
		perror("Error! Could not open file\n");
		return 1; 
	} 

	int rtable_size = get_rtable_size(fp);
	rtable = malloc(sizeof (struct route_table_entry) * rtable_size);
	DIE(rtable == NULL, "memory");

	read_rtable(rtable, rtable_size, fp);
	sort_rtable(rtable, rtable_size);

	init(argc - 2, argv + 2);

	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		
		// Get router IP Address
		struct in_addr ip_addr;
		inet_aton(get_interface_ip(m.interface), &ip_addr);
		uint32_t router_ip_addr = ip_addr.s_addr;
		
		// Get router MAC Address
		uint8_t router_mac_addr[6];
		get_interface_mac(m.interface, router_mac_addr);

		struct arp_header *arp_hdr = parse_arp(m.payload);
		if (arp_hdr != NULL) {
			if (ntohs(arp_hdr->op) == ARPOP_REQUEST) {
				// The router receives an ARP REQUEST
				struct ether_header *eth_hdr = (struct ether_header *) m.payload;

				// Check if the target IP address is the router's address
				if (arp_hdr->tpa == router_ip_addr) {
					// Create ARP REPLY packet
					// Insert router's MAC address in the ethernet header
					memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, sizeof(eth_hdr->ether_dhost));
       				memcpy(eth_hdr->ether_shost, router_mac_addr, sizeof(eth_hdr->ether_shost));
					send_arp(arp_hdr->spa, router_ip_addr, eth_hdr, m.interface, htons(ARPOP_REPLY));
				} 
			} else {
				// The router receives an ARP REPLY
				// Add new entry in the ARP table
				struct ether_header *eth_hdr = (struct ether_header *) m.payload;

				struct arp_entry *check_entry = get_arp_entry(arp_hdr->spa, arp_table, arp_table_size);
				if (check_entry == NULL) {
					arp_table = (struct arp_entry *) realloc(
						arp_table, sizeof (struct arp_entry) * (arp_table_size + 1));
					arp_table[arp_table_size].ip = arp_hdr->spa;
					memcpy(arp_table[arp_table_size].mac, arp_hdr->sha, 
							sizeof(arp_table[arp_table_size].mac));
					arp_table_size++;
				}
				
				if (arp_hdr->tpa == router_ip_addr) {
					// The ARP REPLY is intended for the router, forward the packet
					// Check if there is any packet to forward
					if (queue_empty(q)) {
						continue;
					}

					// Dequeue packet and forward it
					packet *queue_packet = queue_deq(q);

					packet forward_packet;
					memcpy(&forward_packet, queue_packet, sizeof(forward_packet));
					free(queue_packet);

					struct ether_header *eth_hdr_forward = 
						(struct ether_header *) forward_packet.payload;

					memcpy(eth_hdr_forward->ether_dhost, eth_hdr->ether_shost, 
							sizeof(eth_hdr_forward->ether_dhost));

					memcpy(eth_hdr_forward->ether_shost, eth_hdr->ether_dhost,
						sizeof(eth_hdr_forward->ether_shost));

					send_packet(m.interface, &forward_packet);
				}		
			}	
		} else {
			// The router receives a IP or ICMP packet
			struct iphdr *ip_hdr = (struct iphdr *)(m.payload + IP_OFF);
			struct ether_header *eth_hdr = (struct ether_header *) m.payload;
			struct route_table_entry *best_route = get_best_route(ip_hdr->daddr, rtable, rtable_size);

			if (ip_hdr->ttl <= 1) {
				// TTL <= 1, discard packet 
				send_icmp_error(ip_hdr->saddr, router_ip_addr, router_mac_addr, eth_hdr->ether_shost,
								ICMP_TIME_EXCEEDED, 0, m.interface);
				continue;
			}

			if (ip_checksum(ip_hdr, sizeof(struct iphdr)) != 0) {
				// Bad checksum, discard packet 
				continue;
			}

			// Update TTL and compute new checksum						
			--ip_hdr->ttl;
			ip_hdr->check = 0;
			ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));


			if (ip_hdr->daddr == router_ip_addr) {
				// The packet is intended for the router, reply only if it is an ICMP_ECHO Request
				struct icmphdr *icmp_hdr = parse_icmp(m.payload);
				if (icmp_hdr != NULL) {
					if (icmp_hdr->type == ICMP_ECHO) {
						send_icmp(ip_hdr->daddr, ip_hdr->saddr, router_mac_addr, eth_hdr->ether_shost, 
							ICMP_ECHOREPLY, 0, m.interface, icmp_hdr->un.echo.id, icmp_hdr->un.echo.sequence);
					}
				}
			} else {
				// The packet is not intended for the router, forward it
				if (best_route == NULL) {
					send_icmp_error(ip_hdr->saddr, router_ip_addr, router_mac_addr, eth_hdr->ether_shost,
									ICMP_DEST_UNREACH, 0, m.interface);
					continue;
				}
				
				// Get router IP Address for best route
				struct in_addr ip_addr_router;
				inet_aton(get_interface_ip(best_route->interface), &ip_addr_router);
				uint32_t router_ip_bestroute = ip_addr_router.s_addr;

				// Get router MAC Address
				uint8_t router_mac_bestroute[6];
				get_interface_mac(best_route->interface, router_mac_bestroute);

				struct arp_entry *new_arp = get_arp_entry(best_route->next_hop, arp_table, arp_table_size);
				if (new_arp != NULL) {
					// The IP address is in the ARP Table, send directly
					
					struct ether_header *eth_hdr = (struct ether_header *) m.payload;
					memcpy(eth_hdr->ether_dhost, new_arp->mac, sizeof(eth_hdr->ether_dhost));
					memcpy(eth_hdr->ether_shost, router_mac_bestroute, sizeof(eth_hdr->ether_shost));

					send_packet(best_route->interface, &m);
					
				} else {
					// The IP address is not in the ARP Table, send ARP REQUEST
					
					packet *new_packet = malloc (sizeof(packet));
					memcpy(new_packet, &m, sizeof(packet));
					queue_enq(q, new_packet);

					uint8_t dest_mac[6];
					hwaddr_aton(BROADCAST_MAC_ADDRESS, dest_mac);

					struct ether_header new_eth_hdr;			
					new_eth_hdr.ether_type = htons(ETHERTYPE_ARP);						
					memcpy(new_eth_hdr.ether_shost, router_mac_bestroute, sizeof(new_eth_hdr.ether_shost));
					memcpy(new_eth_hdr.ether_dhost, dest_mac, sizeof(new_eth_hdr.ether_dhost));

					send_arp(best_route->next_hop, router_ip_bestroute, &new_eth_hdr, best_route->interface,
							htons(ARPOP_REQUEST));
				}								 
			}		
		}
	}

	free(rtable);
	free(arp_table);

	fclose(fp);
}
