#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio.hpp>
#include "gui_server.hpp"

using boost::asio::ip::tcp;

std::string __itos(int n) {
	std::string rev = "";
	if (n == 0) {
		return std::string("0");
	}
	while (n > 0) {
		rev += char(n % 10 + 48);
		n /= 10;
	}
	std::string norm = "";
	for (int i = rev.length() - 1; i >= 0; --i) {
		norm += rev[i];
	}
	return norm;
}

std::string hello_message()
{
	std::string message = "Hello, client!\n\r";
	return message;
}


/* *****************
 *  GUI CONNECTION *
 * *****************/
 
 /*
public:
	typedef boost::shared_ptr<gui_connection> pointer;

	static pointer create(boost::asio::io_service&, int);
	tcp::socket& socket();
	void start();
	
private:
	gui_connection(boost::asio::io_service&, int);
	void handle_write(const boost::system::error_code&);
	void empty_handler();
	
	tcp::socket socket_;
	std::string message_;
	boost::asio::deadline_timer t_;
	int counter;
	boost::posix_time::time_duration refresh_rate;
	int current_page;
 */

gui_connection::pointer gui_connection::create(
	boost::asio::io_service& io_service, float refresh_rate)
{
	return pointer(new gui_connection(io_service, refresh_rate));
}

tcp::socket& gui_connection::socket()
{
	return socket_;
}

void gui_connection::start()
{
	message_ = hello_message();

	boost::asio::async_write(socket_, boost::asio::buffer(message_),
		boost::bind(&gui_connection::handle_write, shared_from_this(),
			boost::asio::placeholders::error));
}

gui_connection::gui_connection(boost::asio::io_service& io_service, float refresh_rate)
	: socket_(io_service)
	, t_(io_service)
	, counter(0)
	, current_page(1)
{
	int refresh_rate_sec = int(refresh_rate);
	int refresh_rate_millisec = int(1000. * float(refresh_rate - refresh_rate_sec));
	this->refresh_rate = boost::posix_time::seconds(refresh_rate_sec) + boost::posix_time::millisec(refresh_rate_millisec);
}

void gui_connection::handle_write(const boost::system::error_code& /*error*/)
{
	// clean terminal
	message_ = char(0x1B);
	message_ += 'c';
	
	message_ += "(";
	message_ += __itos(current_page);
	message_ += ") Sending some data, part ";
	message_ += __itos(counter);
	message_ += "\n\r";
	boost::asio::async_write(socket_, boost::asio::buffer(message_),
		boost::bind(&gui_connection::empty_handler, shared_from_this()));
	
	if (counter == 0) {
		t_.expires_from_now(refresh_rate);
	}
	else {
		t_.expires_at(t_.expires_at() + refresh_rate);
	}
	t_.async_wait(boost::bind(&gui_connection::handle_write, shared_from_this(),
		boost::asio::placeholders::error));
	counter++;
}

void gui_connection::empty_handler() {}


/* ************
 * GUI_SERVER *
 * ************/
 
 /*
public:
	gui_server(boost::asio::io_service&, int port, int refresh_rate);

private:
	void start_accept();
	void handle_accept(gui_connection::pointer, const boost::system::error_code&);
	
	tcp::acceptor acceptor_;
	float refresh_rate;
 */

gui_server::gui_server(boost::asio::io_service& io_service, int port, float refresh_rate)
	: acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
	, refresh_rate(refresh_rate)
{
	start_accept();
}


void gui_server::start_accept()
{
	gui_connection::pointer new_connection =
		gui_connection::create(acceptor_.get_io_service(), refresh_rate);
	
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
