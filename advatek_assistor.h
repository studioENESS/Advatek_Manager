#pragma once

#include "defines.h"

class advatek_manager {
public:
	uint8_t ProtVer = 8;
	bool deviceExist(uint8_t * Mac);
	std::string macStr(uint8_t * address);
	std::string ipStr(uint8_t * address);

	std::vector <std::string> networkAdaptors;
	int currentAdaptor = -1;
	
	std::vector<sAdvatekDevice*> connectedDevices;
	std::vector<sAdvatekDevice*> virtualDevices;
	std::vector<sAdvatekDevice*> memoryDevices;

	void clearDevices(std::vector<sAdvatekDevice*> &devices);
	void copyDevice(sAdvatekDevice* fromDevice, sAdvatekDevice* toDevice, bool initialise);
	void copyToMemoryDevice(sAdvatekDevice* fromDevice);
	void pasteFromMemoryDeviceTo(sAdvatekDevice* toDevice);

	void copyToNewVirtualDevice(sAdvatekDevice* fromDevice);
	void addVirtualDevice(boost::property_tree::ptree advatek_device, sImportOptions &importOptions);
	void addVirtualDevice(sImportOptions &importOptions);
	void pasteToNewVirtualDevice();
	void updateDevice(int d);
	void identifyDevice(int d, uint8_t duration);
	void setTest(int d);
	void clearConnectedDevices();
	void bc_networkConfig(int d);
	void poll();
	void process_opPollReply(uint8_t * data);
	void process_opTestAnnounce(uint8_t * data);
	void process_udp_message(uint8_t * data);
	void listen();
	void send_udp_message(std::string ip_address, int port, bool b_broadcast, std::vector<uint8_t> message);
	void unicast_udp_message(std::string ip_address, std::vector<uint8_t> message);
	void broadcast_udp_message(std::vector<uint8_t> message);
	void auto_sequence_channels(int d);
	void process_simple_config(int d);

	void refreshAdaptors();
	void setCurrentAdaptor(int adaptorIndex);
	
	void getJSON(sAdvatekDevice *fromDevice, sImportOptions &importOptions);
	void getJSON(sAdvatekDevice *device, boost::property_tree::ptree &root);
	std::string importJSON(sAdvatekDevice *device, sImportOptions &importOptions);
	std::string importJSON(sAdvatekDevice *device, boost::property_tree::ptree advatek_device, sImportOptions &importOptions);
	void exportJSON(sAdvatekDevice *device, std::string path);
	void exportJSON(std::vector<sAdvatekDevice*> &devices, std::string path);
	std::string validateJSON(boost::property_tree::ptree advatek_devices);

	static const char* RGBW_Order[24];

	IClient* m_pUdpClient;
private:

};
