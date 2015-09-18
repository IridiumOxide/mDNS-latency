#include "data_vector.hpp"
#include <vector>
#include <string>

std::string data_to_string(delay_data dd){
	std::string result = "";
	result += dd.IP;
	result += std::string((20 - dd.IP.length()), ' ');
	int avg = dd.get_average();
	int spaces = avg / 10;
	if(spaces > 40){
		spaces = 40;
	}
	result += std::string(spaces, ' ');
	if(dd.udp_latency == -1){
		result += "???";
	}else{
		result += std::to_string(dd.udp_latency);
	}
	result += " ";
	if(dd.tcp_latency == -1){
		result += "???";
	}else{
		result += std::to_string(dd.tcp_latency);
	}
	result += " ";
	if(dd.icmp_latency == -1){
		result += "???";
	}else{
		result += std::to_string(dd.icmp_latency);
	}
	result += "\n\r";
	return result;
}

// TEST TEST TEST
/*
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
*/
// END OF TEST TEST TEST

delay_data::delay_data(){
	IP = "";
	udp_latency = -1;
	tcp_latency = -1;
	icmp_latency = -1;
}

delay_data::delay_data(std::string& s){
	IP = s;
	udp_latency = -1;
	tcp_latency = -1;
	icmp_latency = -1;
}

bool delay_data::operator<(const delay_data& second) const{
	return get_average() < second.get_average();
}


int delay_data::get_average() const{
	int n = 0;
	int sum = 0;
	if(udp_latency != -1){
		sum += udp_latency;
		n++;
	}
	if(tcp_latency != -1){
		sum += tcp_latency;
		n++;
	}
	if(icmp_latency != -1){
		sum += icmp_latency;
		n++;
	}
	if(n == 0){
		return 999999999;
	}else{
		return sum / n;
	}
}
	


delay_record::delay_record()
	: ssh(false)
	, opoznienia(false)
	{}


void delay_records::add_record(std::string IP, int record_type){
	bool found = false;
	for(unsigned int i = 0; i < drs.size(); ++i){
		if(drs[i].IP == IP){
			found = true;
			if(record_type == SSH_RECORD){
				drs[i].ssh = true;
			}else if(record_type == OPOZNIENIA_RECORD){
				drs[i].opoznienia = true;
			}
		}
	}
	if(!found){
		delay_record new_rec;
		new_rec.IP = IP;
		if(record_type == SSH_RECORD){
			new_rec.ssh = true;
		}else if(record_type == OPOZNIENIA_RECORD){
			new_rec.opoznienia = true;
		}
		drs.push_back(new_rec);
	}
}
