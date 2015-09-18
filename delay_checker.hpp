#ifndef DELAY_CHECKER_H
#define DELAY_CHECKER_H

#include <vector>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <sys/time.h>
#include "data_vector.hpp"

using boost::asio::ip::udp;
using boost::asio::ip::tcp;
using boost::asio::ip::icmp;


class udp_server
{
public:
	udp_server(boost::asio::io_service&, int port);
private:
	
	void start();
	void handle_receive(const boost::system::error_code&, size_t);
	void handle_send(const boost::system::error_code&, size_t);
	
	uint64_t to_send[2];
	static const int max_length = 10000;
	udp::socket socket_;
	udp::endpoint remote_endpoint_;
	char buffer_[max_length];
};


class delay_checker
{
public:
	delay_checker(boost::asio::io_service&, delay_record* in, float del_rate, int udp_port);
	void check(const boost::system::error_code&);
	
	delay_data& get_delay_data();
	
private:

	delay_record* d_record;
	delay_data d_data;
	boost::posix_time::time_duration delays_rate;
	
	boost::asio::deadline_timer check_timer_udp;
	boost::asio::deadline_timer check_timer_tcp;
	boost::asio::deadline_timer check_timer_icmp;
	
	udp::endpoint remote_endpoint_;
	
	void ms_udp(const boost::system::error_code&);
	udp::endpoint udp_endpoint;
	udp::socket udp_socket;
	boost::asio::deadline_timer udp_timeout_timer;
	struct timeval udp_begin;
	char udp_buffer_[20];
	void handle_udp_timeout(const boost::system::error_code&);
	void handle_udp_send(const boost::system::error_code&);
	void handle_udp_success(const boost::system::error_code&);
	
	void ms_tcp(const boost::system::error_code&);
	tcp::endpoint tcp_endpoint;
	tcp::socket tcp_socket;
	boost::asio::deadline_timer tcp_timeout_timer;
	void handle_tcp_timeout(const boost::system::error_code&);
	void handle_tcp_success(const boost::system::error_code&, struct timeval);
	
	void ms_icmp(const boost::system::error_code&);
	icmp::resolver icmp_resolver;
	icmp::socket icmp_socket;
	icmp::endpoint icmp_endpoint;
	boost::asio::deadline_timer icmp_timeout_timer;
	unsigned short icmp_sequence_number;
	struct timeval icmp_begin, icmp_end;
	boost::asio::streambuf icmp_reply_buffer;
	size_t icmp_num_replies;
	void handle_icmp_timeout(const boost::system::error_code&);
	void icmp_receive(const boost::system::error_code&);
	void handle_icmp_receive(size_t);
};


class delay_manager
{
public:
	delay_manager(boost::asio::io_service*, delay_records* records, data_vector* out, float delays_rate, int udp_port);

private:
	void handle_check(const boost::system::error_code&);

	boost::asio::io_service* ioserv;
	delay_records* d_records;
	std::vector<delay_checker*> checkers;
	boost::asio::deadline_timer check_timer;
	boost::posix_time::time_duration delays_rate;
	data_vector* out;
	int udp_port;
	float del_rate;
};
#endif
