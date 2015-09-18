// ICMP wzorowane na tutorialach boost.asio

#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <sys/time.h>
#include <endian.h>
#include "data_vector.hpp"
#include "delay_checker.hpp"
#include "icmp_header.hpp"
#include "ipv4_header.hpp"

using boost::asio::ip::udp;
using boost::asio::ip::tcp;
using boost::asio::ip::icmp;
using namespace boost::asio::ip;

const int tcp_port = 22;
const int timeout_seconds = 5;
const uint64_t usec_in_sec = 1000000;

uint64_t time_difference(struct timeval* begin, struct timeval* end){
    if(end->tv_sec < begin->tv_sec || 
        (end->tv_sec == begin->tv_sec && end->tv_usec < begin->tv_usec)){
            return 0;
    }
    uint64_t difference;
    difference = end->tv_sec - begin->tv_sec;
    difference *= usec_in_sec;
    difference += end->tv_usec - begin->tv_usec;
    return difference;
}

unsigned short global_sequence_number(){
	static unsigned short sqn = 0;
	return ++sqn;
}

/* ************
 * UDP_SERVER *
 * ************/

udp_server::udp_server(boost::asio::io_service& io_service, int port)
	: socket_(io_service, udp::endpoint(udp::v4(), port))
{
	start();
}

void udp_server::start()
{
	socket_.async_receive_from(boost::asio::buffer(buffer_, sizeof(uint64_t)), remote_endpoint_,
		boost::bind(&udp_server::handle_receive, this, boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}


void udp_server::handle_receive(const boost::system::error_code& e, size_t bytes_recvd)
{
	if(e){
		return;
	}
	memcpy(to_send, buffer_, sizeof(uint64_t));
	struct timeval time_now;
	gettimeofday(&time_now, NULL);
    uint64_t usec_now = time_now.tv_sec * usec_in_sec + time_now.tv_usec;
    to_send[1] = htobe64(usec_now);
    memcpy(buffer_, to_send, sizeof(uint64_t) * 2);
    socket_.async_send_to(boost::asio::buffer(buffer_, sizeof(uint64_t) * 2), remote_endpoint_,
		boost::bind(&udp_server::handle_send, this, boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
	start();
}


void udp_server::handle_send(const boost::system::error_code& e, size_t bytes_recvd)
{}


/* ***************
 * DELAY_CHECKER *
 * ***************/
 
// checks delays for one target

delay_checker::delay_checker(boost::asio::io_service& io_serv, delay_record* in, float del_rate, int udp_port)
	: d_record(in), d_data(in->IP)
	, check_timer_udp(io_serv), check_timer_tcp(io_serv), check_timer_icmp(io_serv)
	, udp_endpoint(address_v4::from_string(in->IP), udp_port), udp_socket(io_serv, udp::v4()), udp_timeout_timer(io_serv)
	, tcp_endpoint(address_v4::from_string(in->IP), tcp_port), tcp_socket(io_serv, tcp::v4()), tcp_timeout_timer(io_serv)
	, icmp_resolver(io_serv), icmp_socket(io_serv, icmp::v4()), icmp_timeout_timer(io_serv), icmp_sequence_number(0), icmp_num_replies(0)
{
	icmp::resolver::query icmp_query(icmp::v4(), in->IP, "");
	icmp_endpoint = *icmp_resolver.resolve(icmp_query);
	
	int del_rate_sec = int(del_rate);
	int del_rate_millisec = int(1000. * float(del_rate - del_rate_sec));
	delays_rate = boost::posix_time::seconds(del_rate_sec) + boost::posix_time::millisec(del_rate_millisec);
	
	ms_udp(boost::system::error_code());
	ms_tcp(boost::system::error_code());
	ms_icmp(boost::system::error_code());
}


// UDP CLIENT
void delay_checker::ms_udp(const boost::system::error_code& e){
	if(d_record->opoznienia){
		udp_timeout_timer.expires_from_now(boost::posix_time::seconds(timeout_seconds));
		udp_timeout_timer.async_wait(boost::bind(&delay_checker::handle_udp_timeout, this, boost::asio::placeholders::error));
		gettimeofday(&udp_begin, NULL);
		uint64_t usec_now = udp_begin.tv_sec * usec_in_sec + udp_begin.tv_usec;
		uint64_t to_send = htobe64(usec_now);
		memcpy(udp_buffer_, &to_send, sizeof(uint64_t));
		udp_socket.async_send_to(boost::asio::buffer(udp_buffer_, sizeof(uint64_t)), udp_endpoint,
			boost::bind(&delay_checker::handle_udp_send, this, boost::asio::placeholders::error));
	}else{	
		check_timer_udp.expires_from_now(delays_rate);
		check_timer_udp.async_wait(boost::bind(&delay_checker::ms_udp, this, boost::asio::placeholders::error));
	}
}


void delay_checker::handle_udp_send(const boost::system::error_code& e){
	udp_socket.async_receive_from(boost::asio::buffer(udp_buffer_, sizeof(uint64_t) * 2), remote_endpoint_,
		boost::bind(&delay_checker::handle_udp_success, this, boost::asio::placeholders::error));
}


void delay_checker::handle_udp_timeout(const boost::system::error_code& e){
	if(e){
		return;
	}
	std::cout << "UDP TIMEOUT" << std::endl;
	udp_socket.cancel();
	d_data.udp_latency = -1;
	check_timer_udp.expires_from_now(delays_rate);
	check_timer_udp.async_wait(boost::bind(&delay_checker::ms_udp, this, boost::asio::placeholders::error));
}


void delay_checker::handle_udp_success(const boost::system::error_code& e){
	if(e){
		return;
	}
	struct timeval end;
	gettimeofday(&end, NULL);
	udp_timeout_timer.cancel();
	uint64_t dft = time_difference(&udp_begin, &end);
	dft /= 1000;
	d_data.udp_latency = dft;	
	check_timer_udp.expires_from_now(delays_rate);
	check_timer_udp.async_wait(boost::bind(&delay_checker::ms_udp, this, boost::asio::placeholders::error));
}


// TCP CLIENT
void delay_checker::ms_tcp(const boost::system::error_code& e){
	if(d_record->ssh){
		tcp_socket.close();
		try{			
			struct timeval tcp_begin;
			tcp_timeout_timer.expires_from_now(boost::posix_time::seconds(timeout_seconds));
			tcp_timeout_timer.async_wait(boost::bind(&delay_checker::handle_tcp_timeout, this, boost::asio::placeholders::error));
			gettimeofday(&tcp_begin, NULL);
			tcp_socket.async_connect(tcp_endpoint, 
				boost::bind(&delay_checker::handle_tcp_success, this,
				boost::asio::placeholders::error, tcp_begin));
			
		}catch(const std::exception& e){
			std::cout << e.what() << std::endl;
			d_data.tcp_latency = -1;
		}
	}else{
		check_timer_tcp.expires_from_now(delays_rate);
		check_timer_tcp.async_wait(boost::bind(&delay_checker::ms_tcp, this, boost::asio::placeholders::error));
	}
		
}


void delay_checker::handle_tcp_timeout(const boost::system::error_code& e){
	if(e){
		return;
	}
	std::cout << "TCP TIMEOUT" << std::endl;
	tcp_socket.close();
	d_data.tcp_latency = -1;
	check_timer_tcp.expires_from_now(delays_rate);
	check_timer_tcp.async_wait(boost::bind(&delay_checker::ms_tcp, this, boost::asio::placeholders::error));
}


void delay_checker::handle_tcp_success(const boost::system::error_code& e, struct timeval begin){
	if(e){
		return;
	}
	struct timeval end;
	gettimeofday(&end, NULL);
	tcp_timeout_timer.cancel();
	uint64_t dft = time_difference(&begin, &end);
	dft /= 1000;
	d_data.tcp_latency = dft;
	check_timer_tcp.expires_from_now(delays_rate);
	check_timer_tcp.async_wait(boost::bind(&delay_checker::ms_tcp, this, boost::asio::placeholders::error));
}


// ICMP CLIENT
void delay_checker::ms_icmp(const boost::system::error_code& e){
	if(d_record->opoznienia){
		
		char body[4];
		body[0] = 0x34;
		body[1] = 0x68;
		body[2] = 0x68;
		body[3] = 0x07;
		
		icmp_sequence_number = global_sequence_number();
		
		icmp_header echo_request;
		echo_request.type(icmp_header::echo_request);
		echo_request.code(0);
		echo_request.identifier(0x13);
		echo_request.sequence_number(icmp_sequence_number);
		compute_checksum(echo_request, body, body + 4);
		
		
		boost::asio::streambuf icmp_request_buffer;
		std::ostream os(&icmp_request_buffer);
		os << echo_request << body;
		
		gettimeofday(&icmp_begin, NULL);
		icmp_socket.async_send_to(icmp_request_buffer.data(), icmp_endpoint,
			boost::bind(&delay_checker::icmp_receive, this, boost::asio::placeholders::error));
			
			
		icmp_num_replies = 0;
		icmp_timeout_timer.expires_from_now(boost::posix_time::seconds(timeout_seconds));
		icmp_timeout_timer.async_wait(boost::bind(&delay_checker::handle_icmp_timeout, this, boost::asio::placeholders::error));
	}else{
		check_timer_icmp.expires_from_now(delays_rate);
		check_timer_icmp.async_wait(boost::bind(&delay_checker::ms_icmp, this, boost::asio::placeholders::error));
	}
}


void delay_checker::icmp_receive(const boost::system::error_code& e){
	std::cout << "ICMP RECEIVING..." << std::endl;
	icmp_reply_buffer.consume(icmp_reply_buffer.size());
	icmp_socket.async_receive(icmp_reply_buffer.prepare(65536),
		boost::bind(&delay_checker::handle_icmp_receive, this,
		_2));
}


void delay_checker::handle_icmp_receive(size_t length){
	std::cout << "ICMP RECEIVED" << std::endl;
	icmp_reply_buffer.commit(length);

    std::istream is(&icmp_reply_buffer);
    ipv4_header ipv4_hdr;
    icmp_header icmp_hdr;
    is >> ipv4_hdr >> icmp_hdr;

    if (is && icmp_hdr.type() == icmp_header::echo_reply
          && icmp_hdr.identifier() == 0x13
          && icmp_hdr.sequence_number() == icmp_sequence_number)
    {
		if(icmp_num_replies++ == 0){
			icmp_timeout_timer.cancel();
		}
		gettimeofday(&icmp_end, NULL);
		uint64_t tdf = time_difference(&icmp_begin, &icmp_end);
		tdf /= 1000;
		d_data.icmp_latency = tdf;
	}
	
	icmp_receive(boost::system::error_code());
}


void delay_checker::handle_icmp_timeout(const boost::system::error_code& e){
	if(icmp_num_replies == 0){
		std::cout << "ICMP TIMEOUT" << std::endl;
		d_data.icmp_latency = -1;
	}
	check_timer_icmp.expires_from_now(delays_rate);
	check_timer_icmp.async_wait(boost::bind(&delay_checker::ms_icmp, this, boost::asio::placeholders::error));
}


delay_data& delay_checker::get_delay_data(){
	return d_data;
}

/* ***************
 * DELAY_MANAGER *
 * ***************/

// manages checkers

delay_manager::delay_manager(boost::asio::io_service* io_serv, delay_records* records, data_vector* out, float del_rate, int udp_port)
	: ioserv(io_serv)
	, d_records(records)
	, check_timer(*io_serv)
	, out(out)
	, udp_port(udp_port)
	, del_rate(del_rate)
{
	int del_rate_sec = int(del_rate);
	int del_rate_millisec = int(1000. * float(del_rate - del_rate_sec));
	delays_rate = boost::posix_time::seconds(del_rate_sec) + boost::posix_time::millisec(del_rate_millisec);
	
	handle_check(boost::system::error_code());
}


void delay_manager::handle_check(const boost::system::error_code& e){
	if(e){
		check_timer.expires_from_now(delays_rate);
		check_timer.async_wait(boost::bind(&delay_manager::handle_check,
			this, boost::asio::placeholders::error));
		return;
	}
	
	// get delay data from existing checkers
	data_vector v;
	for(unsigned int i = 0; i < checkers.size(); ++i){
		delay_data x = checkers[i]->get_delay_data();
		v.push_back(x);
	}	
	std::sort(v.begin(), v.end());	
	(*out) = v;
	
	std::cout << "[DELAYMANAGER] " << d_records->drs.size() << std::endl;
	if(d_records->drs.size() != checkers.size()){
		// create new checkers for new records
		for(unsigned int i = checkers.size(); i < d_records->drs.size(); ++i){
			delay_checker* new_checker = new delay_checker(*ioserv, &((*d_records).drs[i]), del_rate, udp_port);
			checkers.push_back(new_checker);
		}
	}
	
	// repeat
	check_timer.expires_from_now(delays_rate);
	check_timer.async_wait(boost::bind(&delay_manager::handle_check,
		this, boost::asio::placeholders::error));
}
