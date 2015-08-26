CXXFLAGS=-Wall -O2
BOOSTLIBS=-lboost_system

all: opoznienia

gui_server.o: gui_server.cpp gui_server.hpp
	g++ $(CXXFLAGS) -c gui_server.cpp $(BOOSTLIBS) 

opoznienia.o: opoznienia.cpp
	g++ $(CXXFLAGS) -c opoznienia.cpp $(BOOSTLIBS)

opoznienia: gui_server.o opoznienia.o
	g++ $(CXXFLAGS) -o opoznienia opoznienia.o gui_server.o $(BOOSTLIBS)

clean:
	rm *.o opoznienia
