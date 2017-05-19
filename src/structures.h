#ifndef STRUCTURES_H // one-time include
#define STRUCTURES_H

#include <string>

typedef enum _ROUTER_STATUS
{
    S_OK,
    E_FAILED,
    E_UNDEFINED,
    E_INVALID_PROTOCOL
} ROUTER_STATUS;


typedef struct _udp_packet_tuple
{
	uint32_t	src_ip;
	uint16_t	src_port;
	uint32_t	dest_ip;
	uint16_t	dest_port;
} udp_packet_tuple;

typedef struct _tcp_packet_tuple
{
	uint32_t	src_ip;
	uint16_t	src_port;
	uint32_t	dest_ip;
	uint16_t	dest_port;
} tcp_packet_tuple;

typedef struct _icmp_packet_tuple
{
	uint32_t	src_ip;
	uint32_t	dest_ip;
} icmp_packet_tuple;



typedef struct _nat_map_entry
{
//	char		entry_id[64];
	std::string		in_interface;
	std::string		out_interface;
	uint16_t	protocol;
	time_t		activity;
	
	union
	{
		udp_packet_tuple inside_udp;
		tcp_packet_tuple inside_tcp;
		icmp_packet_tuple inside_icmp;
	};
	
	union
	{
		udp_packet_tuple outside_udp;
		tcp_packet_tuple outside_tcp;
		icmp_packet_tuple outside_icmp;
	};
	
	// expiration timestamp
} nat_map_entry;


#endif
