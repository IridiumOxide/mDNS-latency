/*
 * TODO:
 * - actual data to display
 */

#include <ctime>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "gui_server.hpp"
#include "data_vector.hpp"

#define ENTRIES_PER_PAGE 20

using boost::asio::ip::tcp;


/* *****************
 *  GUI CONNECTION *
 * *****************/
 
 /*
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
 */


/* PUBLIC */

gui_connection::pointer gui_connection::create(boost::asio::io_service& io_service,
	float refresh_rate, data_vector* data)
{
	return pointer(new gui_connection(io_service, refresh_rate, data));
}

tcp::socket& gui_connection::socket()
{
	return socket_;
}

void gui_connection::start()
{
	// turn off echo, turn on character mode
	message_ = "\377\375\042\377\373\001";
	
	boost::asio::async_write(socket_, boost::asio::buffer(message_),
		boost::bind(&gui_connection::handle_write, shared_from_this(),
			boost::asio::placeholders::error));
	
	socket_.async_read_some(boost::asio::buffer(read_byte),
		boost::bind(&gui_connection::handle_read, shared_from_this(),
			boost::asio::placeholders::error));
}


/* PRIVATE */

gui_connection::gui_connection(boost::asio::io_service& io_service,
	float refresh_rate, data_vector* data)
	: socket_(io_service)
	, t_(io_service)
	, counter(0)
	, current_page(1)
	, data(data)
	, end(false)
{
	int refresh_rate_sec = int(refresh_rate);
	int refresh_rate_millisec = int(1000. * float(refresh_rate - refresh_rate_sec));
	this->refresh_rate = boost::posix_time::seconds(refresh_rate_sec) + boost::posix_time::millisec(refresh_rate_millisec);
}

void gui_connection::handle_write(const boost::system::error_code&)
{
	redraw();
	
	if (counter == 0) {
		t_.expires_from_now(refresh_rate);
	}
	else {
		t_.expires_at(t_.expires_at() + refresh_rate);
	}
	if(!end){
		t_.async_wait(boost::bind(&gui_connection::handle_write, shared_from_this(),
			boost::asio::placeholders::error));
	}
	counter++;
}

void gui_connection::handle_read(const boost::system::error_code&)
{
	if(read_byte[0] == 'q' || read_byte[0] == 'Q'){
		if(current_page > 1){
			--current_page;
			redraw();
		}
	}else if(read_byte[0] == 'a' || read_byte[0] == 'A'){
		++current_page;
		redraw();
	}else if(read_byte[0] == 'e' || read_byte[0] == 'E'){
		end = true;
	}
	
	if(!end){
		socket_.async_read_some(boost::asio::buffer(read_byte),
			boost::bind(&gui_connection::handle_read, shared_from_this(),
				boost::asio::placeholders::error));
	}
}

void gui_connection::empty_handler() {}

void gui_connection::redraw(){
	// TODO: MUTEX OR SOMETHING
	int total_entries = (*data).size();
	int total_pages = ((total_entries - 1) / ENTRIES_PER_PAGE) + 1;
	if(total_pages < 1)
		total_pages = 1;
	
	// clean terminal
	message_ = std::string("\x1B" "c");
	
	// gui info
	message_ += gui_info();
	message_ += "[ Currently on page ";
	message_ += std::to_string(current_page);
	message_ += "/";
	message_ += std::to_string(total_pages);
	message_ += " ] ::: ";
	message_ += std::to_string(counter);
	message_ += " time units since connect";
	message_ +="\n\r------------------------------------------------\n\r";
	
	// actual data
	int first_entry = (current_page - 1) * ENTRIES_PER_PAGE;
	int last_entry = std::min(((current_page * ENTRIES_PER_PAGE) - 1), total_entries - 1);
	for(int i = first_entry; i <= last_entry; ++i){
		message_ += data_to_string((*data)[i]);
	}
	
	boost::asio::async_write(socket_, boost::asio::buffer(message_),
		boost::bind(&gui_connection::empty_handler, shared_from_this()));
}

std::string gui_connection::gui_info()
{
	static const std::string message = "NAVIGATION:  Q - page up ::: A - page down ::: E - quit\n\r";
	return message;
}


/* ************
 * GUI_SERVER *
 * ************/
 
 /*
public:
	gui_server(boost::asio::io_service&, int port,
		float refresh_rate, data_vector*);

private:
	void start_accept();
	void handle_accept(gui_connection::pointer, const boost::system::error_code&);
	
	tcp::acceptor acceptor_;
	float refresh_rate;
	data_vector* data;
 */


/* PUBLIC */

gui_server::gui_server(boost::asio::io_service& io_service, int port,
	float refresh_rate, data_vector* data)
	: acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
	, refresh_rate(refresh_rate)
	, data(data)
{
	start_accept();
}


/* PRIVATE */

void gui_server::start_accept()
{
	gui_connection::pointer new_connection =
		gui_connection::create(acceptor_.get_io_service(), refresh_rate, data);
	
	acceptor_.async_accept(new_connection->socket(),
		boost::bind(&gui_server::handle_accept, this, new_connection,
			boost::asio::placeholders::error));
}

void gui_server::handle_accept(gui_connection::pointer new_connection,
	const boost::system::error_code& error)
{
	if (!error)
	{
		new_connection->start();
	}

	start_accept();
}
