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
	
	delay_data();
	delay_data(std::string&);
	bool operator<(const delay_data&) const;
	int get_average() const;
};

std::string data_to_string(delay_data dd);

typedef std::vector<delay_data> data_vector;

struct delay_record
{
	std::string IP;
	bool ssh;
	bool opoznienia; // udp + icmp
	
	delay_record();
};


enum {
	SSH_RECORD = 42,
	OPOZNIENIA_RECORD = 43,
};

struct delay_records
{
	std::vector<delay_record> drs;
	
	void add_record(std::string IP, int record_type);
};

// testing
// extern data_vector sample_data_vector;
// void dv_init();

#endif
