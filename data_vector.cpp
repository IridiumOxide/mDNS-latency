#include "data_vector.hpp"
#include <vector>
#include <string>

std::string data_to_string(delay_data dd){
	std::string result = "";
	result += dd.IP;
	result += std::string((20 - dd.IP.length()), ' ');
	result += std::to_string(dd.udp_latency);
	result += " ";
	result += std::to_string(dd.tcp_latency);
	result += " ";
	result += std::to_string(dd.icmp_latency);
	result += "\n\r";
	return result;
}

data_vector sample_data_vector;

void dv_init(){
	for(int i = 0; i < 128; ++i){
		delay_data x;
		x.IP = "10.0.0.";
		x.IP += std::to_string(i);
		x.udp_latency = (1 + int(i/50)) * 50;
		x.tcp_latency = x.udp_latency + 2;
		x.icmp_latency = x.udp_latency - 1;
		sample_data_vector.push_back(x);
	}
}
