#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio.hpp>

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

std::string hello_message(int n)
{
	std::string message = "Hello, client number ";
	message += __itos(n);
	message += "!\n\r";
	return message;
	
}

class tcp_connection
	: public boost::enable_shared_from_this<tcp_connection>
{
public:
	typedef boost::shared_ptr<tcp_connection> pointer;

	static pointer create(boost::asio::io_service& io_service, int n)
	{
		return pointer(new tcp_connection(io_service, n));
	}

	tcp::socket& socket()
	{
		return socket_;
	}

	void start()
	{
		message_ = hello_message(client_no);

		boost::asio::async_write(socket_, boost::asio::buffer(message_),
			boost::bind(&tcp_connection::handle_write, shared_from_this(),
				boost::asio::placeholders::error));
	}

private:
	tcp_connection(boost::asio::io_service& io_service, int n)
		: socket_(io_service)
		, t_(io_service)
		, counter(0)
		, client_no(n)
	{
	}

	void handle_write(const boost::system::error_code& /*error*/)
	{
		message_ = "[";
		message_ += __itos(client_no);
		message_ += "] Sending some data, part ";
		message_ += __itos(counter);
		message_ += "\n\r";
		boost::asio::async_write(socket_, boost::asio::buffer(message_),
			boost::bind(&tcp_connection::empty_handler, shared_from_this()));
		if (counter == 0) {
			t_.expires_from_now(boost::posix_time::seconds(1));
		}
		else {
			t_.expires_at(t_.expires_at() + boost::posix_time::seconds(1));
		}
		t_.async_wait(boost::bind(&tcp_connection::handle_write, shared_from_this(),
			boost::asio::placeholders::error));
		counter++;
	}

	void empty_handler() {}

	tcp::socket socket_;
	std::string message_;
	boost::asio::deadline_timer t_;
	int counter;
	int client_no;
};

class tcp_server
{
public:
	tcp_server(boost::asio::io_service& io_service)
		: acceptor_(io_service, tcp::endpoint(tcp::v4(), 13))
		, clients(0)
	{
		start_accept();
	}

private:
	void start_accept()
	{
		tcp_connection::pointer new_connection =
			tcp_connection::create(acceptor_.get_io_service(), clients);

		clients++;

		acceptor_.async_accept(new_connection->socket(),
			boost::bind(&tcp_server::handle_accept, this, new_connection,
				boost::asio::placeholders::error));
	}

	void handle_accept(tcp_connection::pointer new_connection,
		const boost::system::error_code& error)
	{
		if (!error)
		{
			new_connection->start();
		}

		start_accept();
	}

	tcp::acceptor acceptor_;
	int clients;
};

int main()
{
	try
	{
		boost::asio::io_service io_service;
		tcp_server server(io_service);
		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}