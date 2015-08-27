#ifndef DATA_VECTOR_H
#define DATA_VECTOR_H

#include <vector>
#include <string>

struct delay_data
{
	std::string IP;
	int udp_latency;
	int tcp_latency;
	int icmp_latency;
};

std::string data_to_string(delay_data dd);

typedef std::vector<delay_data> data_vector;

// testing
extern data_vector sample_data_vector;
void dv_init();

#endif
