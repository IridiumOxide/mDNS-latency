#include <vector>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "mdns_server.hpp"
#include "mdns_packet.hpp"
#include "data_vector.hpp"

using namespace boost::asio::ip;

const int mdns_port = 5353;
const address_v4 listen_address = address_v4::from_string("0.0.0.0");
const address_v4 mdns_address = address_v4::from_string("224.0.0.251");
const boost::posix_time::time_duration nameq_expire_time = boost::posix_time::seconds(1);
const std::vector<std::string> opoznienia({"_opoznienia", "_udp", "local"});
const std::vector<std::string> ssh({"_ssh", "_tcp", "local"});


/* *************
 * MDNS_SERVER *
 * *************/
 
mdns_server::mdns_server(boost::asio::io_service& io_service, 
	data_vector* data, bool cast_ssh, float discovery_rate)
  : cast_ssh(cast_ssh)
  , name_id(0)
  , socket_(io_service)
  , multicast_endpoint_(mdns_address, mdns_port)
  , discovery_timer(io_service)
  , nameq_timer(io_service)
  {
	// Create the socket and join the multicast group
	udp::endpoint listen_endpoint(listen_address, mdns_port);
    socket_.open(listen_endpoint.protocol());
    socket_.set_option(udp::socket::reuse_address(true));
    socket_.bind(listen_endpoint);
    socket_.set_option(multicast::join_group(mdns_address));
	
	int dscv_rate_sec = int(discovery_rate);
	int dscv_rate_millisec = int(1000. * float(discovery_rate - dscv_rate_sec));
	this->discovery_rate = boost::posix_time::seconds(dscv_rate_sec) + boost::posix_time::millisec(dscv_rate_millisec);
	
	
	get_my_IP();
	
	// get_name runs main loops after getting the name
	get_name();	
}


void mdns_server::get_my_IP() //
{
	// copied from the Internet, seems to work
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;      

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa ->ifa_addr->sa_family==AF_INET) {
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            std::cout << ifa->ifa_name << ": " << addressBuffer << std::endl;
            my_IP = address_v4::from_string(addressBuffer);
         }
    }
    if (ifAddrStruct!=NULL) 
        freeifaddrs(ifAddrStruct);
}


void mdns_server::get_name() //
{
	// send "HOSTNAME._opoznienia._udp.local." query, see if someone responds
	mdns_header header(QUERY, 1, 0);
	mdns_packet packet(header);
	mdns_domain domain(full_name_service());
	mdns_question question(domain, QTYPE_A, QCLASS_INTERNET);
	packet.add_question(question);
	send_packet(packet);
	
	nameq_timer.expires_from_now(nameq_expire_time);
	nameq_timer.async_wait(boost::bind(
		&mdns_server::handle_timeout_name,
		this, boost::asio::placeholders::error));
	
	socket_.async_receive_from(boost::asio::buffer(data_, max_length),
		sender_endpoint_, boost::bind(&mdns_server::handle_get_name, this, boost::asio::placeholders::error, 
		boost::asio::placeholders::bytes_transferred));
}


void mdns_server::handle_get_name(const boost::system::error_code& e, size_t bytes_recvd) //
{
	if(e){
		return;
	}
	// check if received packet is a response with a name we're trying to claim
	std::string sdt(data_, data_ + bytes_recvd);
	std::istringstream stream(sdt);
	mdns_packet packet;
	stream >> packet;
	if(packet.get_header().is_response()){
		std::string service_name = packet.get_answers()[0].get_domain().make_string();
		mdns_domain my_domain(full_name_service());
		if(service_name == my_domain.make_string()){
			// need to get another name
			++name_id;
			mdns_header header(QUERY, 1, 0);
			mdns_packet packet(header);
			mdns_domain domain(full_name_service());
			mdns_question question(domain, QTYPE_A, QCLASS_INTERNET);
			packet.add_question(question);
			send_packet(packet);
			nameq_timer.expires_from_now(nameq_expire_time);
		}
	}
	// Next packets might contain the name
	socket_.async_receive_from(
			boost::asio::buffer(data_, max_length), sender_endpoint_,
			boost::bind(&mdns_server::handle_get_name, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));		
}


void mdns_server::handle_timeout_name(const boost::system::error_code& e) //
{
	if(e){
		return;
	}
	// looks like our name is ok. Cancel async operations.
	std::cout << "Got name: " << full_name() << std::endl;
	socket_.cancel();
	// start discovering network
	handle_discovery(boost::system::error_code());
	// receive packets
	socket_.async_receive_from(
		boost::asio::buffer(data_, max_length), sender_endpoint_,
		boost::bind(&mdns_server::handle_receive, this, boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}


void mdns_server::send_packet(mdns_packet &packet) //
{
	std::ostringstream ss;
	ss << packet;
	std::string message_ = ss.str();
	socket_.async_send_to(boost::asio::buffer(message_), multicast_endpoint_,
		boost::bind(&mdns_server::empty_handler, this));
}


void mdns_server::handle_discovery(const boost::system::error_code& e) //
{
	if(e){
		return;
	}
	
	mdns_header query_header(QUERY, 1, 0);
	
	mdns_packet opoznienia_q_packet(query_header);
	mdns_packet ssh_q_packet(query_header);	
	
	mdns_domain opoznienia_domain(opoznienia);
	mdns_domain ssh_domain(ssh);
	
	mdns_question opoznienia_question(opoznienia_domain, QTYPE_PTR, QCLASS_INTERNET);
	mdns_question ssh_question(ssh_domain, QTYPE_PTR, QCLASS_INTERNET);
	
	opoznienia_q_packet.add_question(opoznienia_question);
	ssh_q_packet.add_question(ssh_question);
	
	send_packet(opoznienia_q_packet);
	send_packet(ssh_q_packet);
	
	discovery_timer.expires_at(discovery_timer.expires_at() + discovery_rate);
	discovery_timer.async_wait(boost::bind(&mdns_server::handle_discovery,
		this, boost::asio::placeholders::error));
}


void mdns_server::handle_receive(const boost::system::error_code&, size_t bytes_recvd)
{
	//create packet from received data
	mdns_packet packet;
	std::string sdt(data_, data_ + bytes_recvd);
	std::istringstream stream(sdt);
	stream >> packet;
	
	if(packet.get_header().is_response()){
		// handle response
		for(unsigned int i = 0; i < packet.get_answers().size(); ++i){
			mdns_answer answer = packet.get_answers()[i];
			// TODODODODODODODOOD
		}
	}else{
		// handle query
		for(unsigned int i = 0; i < packet.get_questions().size(); ++i){
			mdns_question question = packet.get_questions()[i];
			if(question.get_type() == QTYPE_PTR){
				if(question.get_domain() == mdns_domain(opoznienia)){
					mdns_header header(RESPONSE_NOT_AUTH, 0, 1);
					mdns_domain domain(opoznienia);
					mdns_packet packet(header);
					mdns_answer answer(domain, QTYPE_PTR, QCLASS_INTERNET);
					std::string rdata = full_name_service_string();
					answer.set_rdata(rdata);
					packet.add_answer(answer);
					send_packet(packet);
				}else if(cast_ssh && (question.get_domain() == mdns_domain(ssh))){
					mdns_header header(RESPONSE_NOT_AUTH, 0, 1);
					mdns_domain domain(ssh);
					mdns_packet packet(header);
					mdns_answer answer(domain, QTYPE_PTR, QCLASS_INTERNET);
					std::string rdata = full_name();
					rdata += ".";
					rdata += domain.make_string();
					answer.set_rdata(rdata);
					packet.add_answer(answer);
					send_packet(packet);
				}				 
			}else if(question.get_type() == QTYPE_A){
				if(question.get_domain().make_string() == full_name_service_string()){
					mdns_header header(RESPONSE_NOT_AUTH, 0, 1);
					mdns_domain domain(full_name_service());
					mdns_packet packet(header);
					mdns_answer answer(domain, QTYPE_A, QCLASS_INTERNET);
					std::string rdata;
					address_v4::bytes_type ipb = my_IP.to_bytes();
					for(int i = 0; i < 4; ++i){
						rdata += ipb[i];
					}
					answer.set_rdata(rdata);
					packet.add_answer(answer);
					send_packet(packet);
				}
			}
		}
	}
	
	// wait for another packet
	socket_.async_receive_from(
		boost::asio::buffer(data_, max_length), sender_endpoint_,
		boost::bind(&mdns_server::handle_receive, this, boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}


void mdns_server::empty_handler(){} //


std::string mdns_server::full_name() //
{
	std::string name = "op";
	name += std::to_string(name_id);
	return name;
}


std::vector<std::string> mdns_server::full_name_service()
{
	std::vector<std::string> name;
	name.push_back(full_name());
	for(unsigned int i = 0; i < opoznienia.size(); ++i){
		name.push_back(opoznienia[i]);
	}
	return name;
}

std::string mdns_server::full_name_service_string()
{
	mdns_domain my_domain(full_name_service());
	return my_domain.make_string();
}
