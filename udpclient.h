#pragma once

#include <string>
#include <vector>
#include <thread>
#include <array>
#include "LockedQueue.h"
#include <boost/asio.hpp>

static const int NetworkBufferSize = 4096;
using boost::asio::ip::udp;
class IClient 
{
public:
	virtual ~IClient() {};
	virtual bool bind() = 0;
	virtual bool HasMessages() = 0;
	virtual void Send(const std::vector<uint8_t>& message, std::string& s_adr, bool b_broadcast) = 0;
	virtual std::vector<uint8_t> PopMessage() = 0;
};


class UdpClient : public IClient
{

public:
	UdpClient(std::string host, unsigned short server_port, unsigned short local_port = 0);
	virtual ~UdpClient() override;
	virtual bool bind() override;
	virtual bool HasMessages() override;
	virtual void Send(const std::vector<uint8_t>& message, std::string& s_adr, bool b_broadcast) override;
	virtual std::vector<uint8_t> PopMessage() override;
protected:
	void run_service();
	void start_receive();

	void handle_receive(const std::error_code& error, std::size_t bytes_transferred);

private:
	std::string m_ipAddress;
	unsigned short m_serverport;
	boost::asio::io_service io_service;
	boost::asio::io_context io_context;
	udp::socket socket, recvsocket;
	udp::endpoint server_endpoint;
	udp::endpoint remote_endpoint;
	std::array<char, NetworkBufferSize> recv_buffer;
	std::thread service_thread;
	LockedQueue<std::vector<uint8_t>> incomingMessages;
};
