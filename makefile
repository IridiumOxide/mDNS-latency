CXXFLAGS=-Wall -O2 -std=c++11
BOOSTLIBS=-lboost_system -pthread

all: opoznienia

data_vector.o: data_vector.cpp data_vector.hpp
	g++ $(CXXFLAGS) -c data_vector.cpp $(BOOSTLIBS)

mdns_server.o: mdns_server.cpp mdns_server.hpp mdns_packet.hpp data_vector.o
	g++ $(CXXFLAGS) -c mdns_server.cpp $(BOOSTLIBS)

delay_checker.o: delay_checker.cpp delay_checker.hpp data_vector.o icmp_header.hpp ipv4_header.hpp
	g++ $(CXXFLAGS) -c delay_checker.cpp $(BOOSTLIBS)

gui_server.o: gui_server.cpp gui_server.hpp data_vector.o
	g++ $(CXXFLAGS) -c gui_server.cpp $(BOOSTLIBS) 

opoznienia.o: opoznienia.cpp data_vector.o
	g++ $(CXXFLAGS) -c opoznienia.cpp $(BOOSTLIBS)

opoznienia: gui_server.o opoznienia.o data_vector.o mdns_server.o delay_checker.o
	g++ $(CXXFLAGS) -o opoznienia opoznienia.o gui_server.o data_vector.o mdns_server.o delay_checker.o $(BOOSTLIBS)

clean:
	rm *.o opoznienia
