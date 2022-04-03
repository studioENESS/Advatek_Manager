#include "udpclient.h"
#include <iostream>
typedef boost::asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO> rcv_timeout_option; //somewhere in your headers to be used everywhere you need it
//...

UdpClient::UdpClient(std::string host, unsigned short server_port, unsigned short local_port)
    : recvsocket(io_service, udp::endpoint(udp::v4(), server_port))
    , socket(io_service, udp::endpoint(udp::v4(), 0))
    , m_ipAddress(host)
    , m_serverport(server_port)
{
    udp::resolver resolver(io_service);
    udp::resolver::query query(udp::v4(), host, std::to_string(2143));
    server_endpoint = *resolver.resolve(query);
    socket = boost::asio::ip::udp::socket(io_context, server_endpoint.protocol());

    socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    recvsocket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    //socket.set_option(boost::asio::ip::udp::socket::{ 200 });


    socket.bind(server_endpoint);

    service_thread = std::thread(&UdpClient::run_service, this);
}

UdpClient::~UdpClient()
{
    socket.close();
    io_service.stop();
    service_thread.join();

}

bool UdpClient::bind()
{
    bool bOk = true;
    return bOk;
}
bool UdpClient::HasMessages()
{
    return !incomingMessages.empty();
}

void UdpClient::Send(const std::vector<uint8_t>& message, std::string& s_adr, bool b_broadcast)
{
    boost::asio::ip::address_v4 local_interface = boost::asio::ip::address_v4::from_string(m_ipAddress.c_str());
    boost::asio::ip::multicast::outbound_interface option(local_interface);
    socket.set_option(option);

    socket.set_option(boost::asio::socket_base::broadcast(b_broadcast));
    //remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(s_adr.c_str()), m_serverport);
    boost::asio::ip::udp::endpoint sendpoint(boost::asio::ip::address::from_string(s_adr.c_str()), m_serverport);

    //sock.send_to(boost::asio::buffer(message), sendpoint);

    socket.send_to(boost::asio::buffer(message), sendpoint);
    //start_receive();
}

std::vector<uint8_t> UdpClient::PopMessage()
{
    if (incomingMessages.empty())
        throw std::logic_error("No messages to pop");
    return incomingMessages.pop();
}

void UdpClient::run_service()
{
    while(socket.is_open())
        start_receive();
}

void UdpClient::start_receive()
{
    try
    {
        
		size_t availBytes = recvsocket.available();
        if (availBytes > 0)
        {
            uint8_t buffer[100000];
            std::size_t bytes_transferred = recvsocket.receive_from(boost::asio::buffer(buffer,10000), remote_endpoint);
            if (bytes_transferred > 1)
            {
                std::vector<uint8_t> message;
                for (int i = 0; i < bytes_transferred; i++)
                {
                    message.push_back(buffer[i]);
                }
                incomingMessages.push(message);
            }
        }
    }
    catch (const boost::system::system_error& ex)
    {
        std::cout << "Failed to receive from socket ... " << std::endl;
        std::cout << ex.what() << std::endl;
    }
}

void UdpClient::handle_receive(const std::error_code& error, std::size_t bytes_transferred)
{
    if (!error)
    {
        std::vector<uint8_t> message(recv_buffer.data(), recv_buffer.data() + bytes_transferred);
        incomingMessages.push(message);
        //statistics.RegisterReceivedMessage(bytes_transferred);
    }
    else
    {
        //Log::Error("Client::handle_receive:", error);
    }

    start_receive();
}

