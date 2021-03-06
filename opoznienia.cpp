#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <sstream>
#include "gui_server.hpp"
#include "data_vector.hpp"
#include "mdns_server.hpp"
#include "delay_checker.hpp"

using boost::asio::ip::tcp;

int parse_int(char* x){
	std::string str(x);
	for(unsigned int i = 0; i < str.length(); ++i){
		if(!isdigit(str[i])){
			return -1;
		}
	}
	std::istringstream i(str);
	int result;
	if(!(i >> result) || result < 1){
		return -1;
	}
	return result;
}

float parse_float(char* x){
	std::string str(x);
	bool hasdot = false;
	for(unsigned int i = 0; i < str.length(); ++i){
		if(!isdigit(str[i])){
			if(str[i] == '.'){
				if(hasdot){
					return -1;
				}else{
					hasdot = true;
				}
			}else{
				return -1;
			}
		}
	}
	std::istringstream i(str);
	float result;
	if(!(i >> result) || result <= 0){
		return -1;
	}
	return result;
}


inline int get_pos_int_parameter(int dif, char* x){
	if(dif == 1){
		std::cout << "Expected positive int, got nothing" << std::endl;
		exit(1);
	}
	int result = parse_int(x);
	if(result == -1){
		std::cout << "Expected positive int, got " << x << std::endl;
		exit(1);
	}
	return result;
}

inline float get_pos_float_parameter(int dif, char* x){
	if(dif == 1){
		std::cout << "Expected positive float, got nothing" << std::endl;
		exit(1);
	}
	float result = parse_float(x);
	if(result == -1){
		std::cout << "Expected positive float, got " << x << std::endl;
		exit(1);
	} 
	return result;
}


int main(int argc, char* argv[])
{
	int udp_delay_port = 3382;
	int ui_port = 3637;
	float delay_cooldown_time = 1;
	float discovery_cooldown_time = 10;
	float ui_refresh_time = 1; 
	bool cast_ssh = false;
	
	for(int i = 1; i < argc; i++){
		int dif = argc - i;
		if(strcmp(argv[i], "-u") == 0){
			udp_delay_port = get_pos_int_parameter(dif, argv[++i]);
		}else if(strcmp(argv[i], "-U") == 0){
			ui_port = get_pos_int_parameter(dif, argv[++i]);
		}else if(strcmp(argv[i], "-t") == 0){
			delay_cooldown_time = get_pos_float_parameter(dif, argv[++i]);
		}else if(strcmp(argv[i], "-T") == 0){
			discovery_cooldown_time = get_pos_float_parameter(dif, argv[++i]);
		}else if(strcmp(argv[i], "-v") == 0){
			ui_refresh_time = get_pos_float_parameter(dif, argv[++i]);
		}else if(strcmp(argv[i], "-s") == 0){
			cast_ssh = true;
		}else{
			std::cout << "Invalid parameter " << argv[i] << std::endl;
			return 1;
		}
	}
	
	std::cout << udp_delay_port << std::endl;
	std::cout << delay_cooldown_time << std::endl;
	std::cout << discovery_cooldown_time << std::endl;
	std::cout << cast_ssh << std::endl;

	// TEST
	// dv_init();
	
	data_vector print_data;
	delay_records address_data;
	
	try{
		boost::asio::io_service io_service;
		mdns_server server_mdns(io_service, &address_data, cast_ssh, discovery_cooldown_time);
		std::cout << "MDNS sever up" << std::endl;
		udp_server server_udp(io_service, udp_delay_port);
		std::cout << "UDP server up" << std::endl;
		delay_manager server_delay(&io_service, &address_data, &print_data, delay_cooldown_time, udp_delay_port);
		std::cout << "Delay checker up" << std::endl;
		gui_server server_gui(io_service, ui_port, ui_refresh_time, &print_data);
		std::cout << "UI available on port " << ui_port << std::endl;
		io_service.run();
	}
	catch (std::exception& e){
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
