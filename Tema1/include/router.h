#ifndef ROUTER_H_   
#define ROUTER_H_

#define IP_OFF (sizeof(struct ether_header))
#define ICMP_OFF (IP_OFF + sizeof(struct iphdr))
#define BROADCAST_MAC_ADDRESS "ff:ff:ff:ff:ff:ff"


/* Routing table entry */
struct route_table_entry {
	uint32_t prefix;
	uint32_t next_hop;
	uint32_t mask;
	int interface;
};

/* ARP table entry */
struct arp_entry {
	__u32 ip;         /* IP address. */
	uint8_t mac[6];   /* Hardware address. */
};


/**
 * @brief Return number of entries in the routing table
 * 
 * @param fp                - file pointer
 * @return int              - number of entries
 */
int get_rtable_size(FILE *fp){
	int lines = 0;
	char ch;

	while(!feof(fp)){
		ch = fgetc(fp);
		if(ch == '\n') {
			lines++;
		}
	}

	return lines;
}

/**
 * @brief Parse function for routing table
 *  
 * @param rtable            - routing table
 * @param rt_size           - number of entries in the routing table
 * @param fp                - file pointer
 */
void read_rtable(struct route_table_entry *rtable, int rt_size, FILE* fp) {
	
	fseek(fp, 0, SEEK_SET);
	int count = 0;

	for (int i = 0; i < rt_size; i++){
		char prefix[16];
		char next_hop[16];
		char mask[16];
		char interface[2];
			
		fscanf(fp, "%s %s %s %s", prefix, next_hop, mask, interface);
		struct in_addr prefix_addr;
		struct in_addr next_hop_addr;
		struct in_addr mask_addr;
		
		inet_aton(prefix, &prefix_addr);
		inet_aton(next_hop, &next_hop_addr);
		inet_aton(mask, &mask_addr);

		rtable[count].prefix = prefix_addr.s_addr;
		rtable[count].next_hop = next_hop_addr.s_addr;
		rtable[count].mask = mask_addr.s_addr;
		rtable[count].interface = atoi(interface);

		count++;
	}
}

/**
 * @brief Sort routing table entries ascending by prefix, then by mask
 * 
 * @param rtable           - routing table
 * @param rt_size          - number of entries in the routing table
 */
void sort_rtable(struct route_table_entry *rtable, int rt_size){
	for(int i = 0; i < rt_size - 1; i ++) {
		for (int j = i + 1; j < rt_size; j++) {
			if (rtable[i].prefix > rtable[j].prefix) {
				struct route_table_entry entry;
				memcpy(&entry, &rtable[i], sizeof(entry));
				memcpy(&rtable[i], &rtable[j], sizeof(entry));
				memcpy(&rtable[j], &entry, sizeof(entry));
			} else if (rtable[i].prefix == rtable[j].prefix){
				if(rtable[i].mask > rtable[j].mask) {
					struct route_table_entry entry;
					memcpy(&entry, &rtable[i], sizeof(entry));
					memcpy(&rtable[i], &rtable[j], sizeof(entry));
					memcpy(&rtable[j], &entry, sizeof(entry));
				}
			}
		}
	}
}

/**
 * @brief  Returns a pointer (eg. &arp_table[i]) to the best matching ARP entry.
            for the given dest_ip or NULL if there is no matching entry.
 * 
 * @param ip_addr               - IP address that is looked for
 * @param arp_table             - ARP table
 * @param arp_table_size        - number of entries in the ARP table
 * @return struct arp_entry*    - ARP entry that has given IP address
 */
struct arp_entry *get_arp_entry(uint32_t ip_addr, struct arp_entry *arp_table, int arp_table_size) {
	
	for (int i = 0; i < arp_table_size; i++) {
    	if (ip_addr == arp_table[i].ip) {
    		return &arp_table[i];
    	}
  	}

	return NULL;
}


/**
 * @brief Returns the best route for a packet
 * 
 * @param dest_ip                       - destination IP address
 * @param rtable                        - routing table
 * @param rtable_size                   - number of entries in the routing table
 * @return struct route_table_entry*    - routing table entry - best route
 */
struct route_table_entry *get_best_route(__u32 dest_ip, struct route_table_entry *rtable, int rtable_size) {

	int index = -1;
	int low = 0, high = rtable_size;

	while(low <= high) {
		int mid = (low + high) / 2;
        
		if (((dest_ip & rtable[mid].mask) > rtable[mid].prefix)) {
			low = mid + 1;
		} else if (((dest_ip & rtable[mid].mask) < rtable[mid].prefix)) {
			high = mid - 1;
		} else {
			index = mid;
			low = mid + 1;
		}
	}

  	if (index == -1) {
    	return NULL;
  	}

  	return &rtable[index];
}


/**
 * @brief Forward packet when a packed is not intended for the router
 * 
 * @param dest_addr         - destination IP address
 * @param src_addr          - sender IP address
 * @param rtable            - routing table
 * @param rtable_size       - number of entries in the routing table
 * @param arp_table         - ARP table
 * @param arp_table_size    - number of entries in the ARP table
 * @param m                 - packet to forward
 * @param q                 - queue
 */
void forward_message(uint32_t dest_addr, uint32_t src_addr, struct route_table_entry *rtable, int rtable_size,
 				struct arp_entry *arp_table, int arp_table_size, packet m, queue q);


#endif // ROUTER_H_
