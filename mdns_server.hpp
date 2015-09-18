#ifndef MDNS_SERVER_H
#define MDNS_SERVER_H

#include <boost/asio.hpp>
#include "data_vector.hpp"
#include "mdns_packet.hpp"

using namespace boost::asio::ip;

class mdns_server
{
public:
	mdns_server(boost::asio::io_service&, delay_records*, bool cast_ssh, float discovery_rate);

private:
	void get_my_IP();
	void get_name();
	void handle_get_name(const boost::system::error_code&, size_t);
	void handle_timeout_name(const boost::system::error_code&);
	
	void send_packet(mdns_packet&);
	void handle_discovery(const boost::system::error_code&);
	void handle_receive(const boost::system::error_code&, size_t);
	void empty_handler();
	
	std::string full_name();
	std::vector<std::string> full_name_service();
	std::string full_name_service_string();
	
	bool cast_ssh;
	int name_id;
	address_v4 my_IP;
	delay_records* network_data;
	udp::socket socket_;
	udp::endpoint sender_endpoint_;
	udp::endpoint multicast_endpoint_;
	boost::posix_time::time_duration discovery_rate;
	boost::asio::deadline_timer discovery_timer;
	boost::asio::deadline_timer nameq_timer;
	enum { max_length = 100000 };
	char data_[max_length];
};

#endif
