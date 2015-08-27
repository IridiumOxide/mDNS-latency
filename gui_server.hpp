#ifndef GUI_SERVER_H
#define GUI_SERVER_H

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include "data_vector.hpp"

using boost::asio::ip::tcp;

class gui_connection 
	: public boost::enable_shared_from_this<gui_connection>
{
public:
	typedef boost::shared_ptr<gui_connection> pointer;
	
	static pointer create(boost::asio::io_service&,
		float refresh_rate, data_vector*);
	tcp::socket& socket();
	void start();
	
private:
	gui_connection(boost::asio::io_service&, 
		float refresh_rate, data_vector*);
	void handle_write(const boost::system::error_code&);
	void handle_read(const boost::system::error_code&);
	void empty_handler();
	void redraw();
	std::string gui_info();
	
	tcp::socket socket_;
	std::string message_;
	boost::asio::deadline_timer t_;
	int counter;
	boost::posix_time::time_duration refresh_rate;
	int current_page;
	char read_byte[1];
	data_vector* data;
	bool end;
};

class gui_server
{
public:
	gui_server(boost::asio::io_service&, int port,
		float refresh_rate, data_vector*);

private:
	void start_accept();
	void handle_accept(gui_connection::pointer, const boost::system::error_code&);
	
	tcp::acceptor acceptor_;
	float refresh_rate;
	data_vector* data;
};

#endif
