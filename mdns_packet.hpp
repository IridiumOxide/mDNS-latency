#ifndef MDNS_PACKET_H
#define MDNS_PACKET_H

#include <vector>
#include <iostream>
#include <endian.h>

enum{
	QUERY = 0x0000,
	RESPONSE_NOT_AUTH = 0x8000,
	RESPONSE_AUTH = 0x8400,
};

enum{
	QCLASS_INTERNET = 0x0001,
	QCLASS_ANY = 0x00FF,
};

enum{
	QTYPE_A = 0x0001,
	QTYPE_TXT = 0x0010,
	QTYPE_PTR = 0x000c,
	QTYPE_SRV = 0x0021,
	QTYPE_ALL = 0x00FF
};


class mdns_header
{
public:
	static const int header_length = 12;	
	
	friend std::istream& operator>>(std::istream &s, mdns_header &header){
		s.read(reinterpret_cast<char*>(header.rep_), mdns_header::header_length);
		return s;
	}
	
	friend std::ostream& operator<<(std::ostream &s, mdns_header &header){
		s.write(reinterpret_cast<char*>(header.rep_), mdns_header::header_length);
		return s;
	}
	
	bool is_response(){
		return (get_flags() & RESPONSE_NOT_AUTH);
	}

	mdns_header(){ std::fill(rep_, rep_ + sizeof(rep_), 0); }

	mdns_header(unsigned short flags, unsigned short qdcount = 0, unsigned short ancount = 0){
		std::fill(rep_, rep_ + sizeof(rep_), 0);
		set_flags(flags);
		set_qdcount(qdcount);
		set_ancount(ancount);
	}
	
	unsigned short get_id(){ return decode(0, 1); }
	unsigned short get_flags(){ return decode(2, 3); }
	unsigned short get_qdcount(){ return decode(4, 5); }
	unsigned short get_ancount(){ return decode(6, 7); }
	unsigned short get_nscount(){ return decode(8, 9); }
	unsigned short get_arcount(){ return decode(10, 11); }
	
	void set_id(unsigned short n){ encode(0, 1, n); }
	void set_flags(unsigned short n){ encode(2, 3, n); }
	void set_ancount(unsigned short n){ encode(4, 5, n); }
	void set_qdcount(unsigned short n){ encode(6, 7, n); }
	void set_nscount(unsigned short n){ encode(8, 9, n); }
	void set_arcount(unsigned short n){ encode(10, 11, n); }
	
private:
	// we need encode/decode, because data is in big endian
    unsigned short decode(int a, int b) const { return (rep_[a] << 8) + rep_[b]; }    
    void encode(int a, int b, unsigned short n) { 
		rep_[a] = (unsigned char)(n / 256);
		rep_[b] = (unsigned char)(n % 256);
	}
    
    unsigned char rep_[header_length];	
};


class mdns_domain
{
public:
	friend std::istream& operator>>(std::istream &s, mdns_domain &domain){
		unsigned char x;
		s.read(reinterpret_cast<char*>(&x), sizeof(char));
		while(x > 0){
			char buf[256];
			for(unsigned char i = 0; i < x; ++i){
				s.read(reinterpret_cast<char*>(&buf[i]), sizeof(char));
			}
			domain.names.push_back(std::string(buf, buf + x));
		}
		return s;
	}
	
	
	friend std::ostream& operator<<(std::ostream &s, mdns_domain &domain){
		unsigned char x;
		for(unsigned int i = 0; i < domain.names.size(); ++i){
			x = domain.names[i].length();
			s.write(reinterpret_cast<char*>(&x), sizeof(char));
			s << domain.names[i];
		}
		x = 0;
		s << x;
		return s;
	}
	
	
	std::string make_string()
	{
		std::string result = ".";
		for(unsigned int i = 0; i < names.size(); ++i){
			result += names[i];
			result += '.';
		}
		return result;
	}
	
	
	mdns_domain(){}
	
	
	mdns_domain(std::vector<std::string> names)
	: names(names)
	{}
	
	
	bool operator==(const mdns_domain &d) const{
		return names == d.names;
	}
	
private:
	std::vector<std::string> names;
};


class mdns_question
{
public:

	friend std::istream& operator>>(std::istream &s, mdns_question &question){
		s >> question.domain;
		unsigned short n;
		s.read(reinterpret_cast<char*>(&n), sizeof(short));
		question.qtype = be16toh(n);
		s.read(reinterpret_cast<char*>(&n), sizeof(short));
		question.qclass = be16toh(n);
		
		return s;
	}
	
	friend std::ostream& operator<<(std::ostream &s, mdns_question &question){
		s << question.domain;
		unsigned short n;
		n = htobe16(question.qtype);
		s.write(reinterpret_cast<char*>(&n), sizeof(short));
		n = htobe16(question.qclass);
		s.write(reinterpret_cast<char*>(&n), sizeof(short));
		
		return s;
	} 
	
	mdns_question(){}
	
	mdns_question(mdns_domain domain, unsigned short qtype, unsigned short qclass)
	: domain(domain)
	, qtype(qtype)
	, qclass(qclass)
	{}
	
	unsigned short get_type(){
		return qtype;
	}
	
	mdns_domain& get_domain(){
		return domain;
	}
private:
	mdns_domain domain;
	unsigned short qtype;
	unsigned short qclass;
};


class mdns_answer
{
public:
	friend std::istream& operator>>(std::istream &s, mdns_answer &answer){
		s >> answer.domain;
		unsigned short n;
		uint32_t m;
		s.read(reinterpret_cast<char*>(&n), sizeof(short));
		answer.qtype = be16toh(n);
		s.read(reinterpret_cast<char*>(&n), sizeof(short));
		answer.qclass = be16toh(n);
		s.read(reinterpret_cast<char*>(&m), sizeof(uint32_t));
		answer.ttl = be32toh(m);
		s.read(reinterpret_cast<char*>(&n), sizeof(short));
		answer.rdlength = be16toh(n);
		for(unsigned short i = 0; i < answer.rdlength; ++i){
			char x;
			s.read(reinterpret_cast<char*>(&x), sizeof(char));
			answer.rdata.push_back(x);
		}
		
		return s;
	}
	
	friend std::ostream& operator<<(std::ostream &s, mdns_answer &answer){
		s << answer.domain;
		unsigned short n;
		uint32_t m;
		n = htobe16(answer.qtype);
		s.write(reinterpret_cast<char*>(&n), sizeof(short));
		n = htobe16(answer.qclass);
		s.write(reinterpret_cast<char*>(&n), sizeof(short));
		m = htobe32(answer.ttl);
		s.write(reinterpret_cast<char*>(&m), sizeof(uint32_t));
		n = htobe16(answer.rdlength);
		s.write(reinterpret_cast<char*>(&n), sizeof(short));
		for(unsigned short i = 0; i < answer.rdlength; ++i){
			s.write(reinterpret_cast<char*>(&(answer.rdata[i])), sizeof(char));
		}
		
		return s;
	}
	
	mdns_answer(){}
	
	mdns_answer(mdns_domain domain, unsigned short qtype, unsigned short qclass,
		unsigned long ttl = 30)
	: domain(domain)
	, qtype(qtype)
	, qclass(qclass)
	, ttl(ttl)
	{}
	
	mdns_domain& get_domain(){
		return domain;
	}
	
	void set_rdata(std::string sdt){
		for(unsigned int i = 0; i < sdt.length(); ++i){
			rdata.push_back(sdt[i]);
		}
		rdlength = rdata.size();
	}
	
	unsigned short get_type(){
		return qtype;
	}
	
private:
	mdns_domain domain;
	unsigned short qtype;
	unsigned short qclass;
	uint32_t ttl;
	unsigned short rdlength;
	std::vector<unsigned char> rdata;
};


class mdns_packet
{
	
public:
	friend std::istream& operator>>(std::istream &s, mdns_packet &packet){
		s >> packet.header;
		for(int i = 0; i < packet.header.get_qdcount(); ++i){
			mdns_question question;
			s >> question;
			packet.questions.push_back(question);
		}
		for(int i = 0; i < packet.header.get_ancount(); ++i){
			mdns_answer answer;
			s >> answer;
			packet.answers.push_back(answer);
		}
		return s;
	}
	
	friend std::ostream& operator<<(std::ostream &s, mdns_packet &packet){
		s << packet.header;
		for(unsigned int i = 0; i < packet.questions.size(); ++i){
			s << packet.questions[i];
		}
		for(unsigned int i = 0; i < packet.answers.size(); ++i){
			s << packet.answers[i];
		}
		return s;
	}
	
	void add_question(mdns_question question){
		questions.push_back(question);
		header.set_qdcount(header.get_qdcount() + 1);
	}	
	
	void add_answer(mdns_answer answer){
		answers.push_back(answer);
		header.set_ancount(header.get_ancount() + 1);
	}
	
	mdns_header& get_header(){
		return header;
	}
	
	std::vector<mdns_answer>& get_answers(){
		return answers;
	}
	
	std::vector<mdns_question>& get_questions(){
		return questions;
	}
	
	mdns_packet(const mdns_header &header)
	: header(header)
	{}
	
	mdns_packet(){}

private:
	mdns_header header;
	std::vector<mdns_question> questions;
	std::vector<mdns_answer> answers;
};

#endif
