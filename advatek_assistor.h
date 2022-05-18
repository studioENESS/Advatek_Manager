#pragma once

#include "defines.h"

namespace pt = boost::property_tree;

class advatek_manager {
public:
	uint8_t ProtVer = 8;

	pt::ptree OutputPixels;
	pt::ptree OutputUniv;
	pt::ptree OutputChan;
	pt::ptree OutputNull;
	pt::ptree OutputZig;
	pt::ptree OutputReverse;
	pt::ptree OutputColOrder;
	pt::ptree OutputGrouping;
	pt::ptree OutputBrightness;
	pt::ptree DmxOutOn;
	pt::ptree DmxOutUniv;
	pt::ptree DriverType;
	pt::ptree DriverSpeed;
	pt::ptree DriverExpandable;
	pt::ptree DriverNames;
	pt::ptree Gamma;

	const char* advatek_manager::SortTypes[3] = {
		"Nickname",
		"Static IP",
		"Current IP"
	};

	int sortTypeConnected = 0;
	int sortTypeVirtual = 0;

	bool deviceExist(std::vector<sAdvatekDevice*>& devices, uint8_t * Mac);
	bool ipInRange(std::string ip, sAdvatekDevice* device);
	bool sameNetworkSettings(sAdvatekDevice* fromDevice, sAdvatekDevice* toDevice);
	bool deviceCompatible(sAdvatekDevice* fromDevice, sAdvatekDevice* toDevice);
	bool devicesInSync(sAdvatekDevice* fromDevice, sAdvatekDevice* toDevice);

	std::string macStr(uint8_t * address);
	std::string ipStr(uint8_t * address);

	std::vector <std::string> networkAdaptors;
	int currentAdaptor = -1;
	size_t getConnectedDeviceIndex(std::string mac);

	std::vector<sAdvatekDevice*> connectedDevices;
	std::vector<sAdvatekDevice*> virtualDevices;
	std::vector<sAdvatekDevice*> memoryDevices;
	std::vector<sAdvatekDevice*> getDevicesWithStaticIP(std::vector<sAdvatekDevice*>& devices, std::string ipstr);
	std::vector<sAdvatekDevice*> getDevicesWithNickname(std::vector<sAdvatekDevice*>& devices, std::string nickname);
	std::vector<sAdvatekDevice*> getDevicesWithMac(std::vector<sAdvatekDevice*>& devices, std::string mac);

	void removeConnectedDevice(size_t index);
	void removeConnectedDevice(std::string mac);
	void sortDevices(std::vector<sAdvatekDevice*> &devices, int sortType);
	void sortAllDevices();
	void clearDevices(std::vector<sAdvatekDevice*> &devices);
	void copyDevice(sAdvatekDevice* fromDevice, sAdvatekDevice* toDevice, bool initialise);
	void copyToMemoryDevice(sAdvatekDevice* fromDevice);
	void pasteFromMemoryDeviceTo(sAdvatekDevice* toDevice);

	void copyToNewVirtualDevice(sAdvatekDevice* fromDevice);
	void addVirtualDevice(boost::property_tree::ptree advatek_device, sImportOptions &importOptions);
	void addVirtualDevice(sImportOptions &importOptions);
	void pasteToNewVirtualDevice();
	void updateDeviceWithMac(sAdvatekDevice* device, uint8_t* Mac, std::string ipStr);
	void updateDevice(int d);
	void updateConnectedDevice(sAdvatekDevice* fromDevice, sAdvatekDevice* connectedDevice);
	void identifyDevice(int d, uint8_t duration);
	void setTest(int d);
	void clearConnectedDevices();
	void bc_networkConfig(int d);
	void bc_networkConfig(sAdvatekDevice* device);
	void poll();
	void softPoll();
	void process_opPollReply(uint8_t * data);
	void process_opTestAnnounce(uint8_t * data);
	void process_udp_message(uint8_t * data);
	void listen();
	void send_udp_message(std::string ip_address, int port, bool b_broadcast, std::vector<uint8_t> message);
	void unicast_udp_message(std::string ip_address, std::vector<uint8_t> message);
	void broadcast_udp_message(std::vector<uint8_t> message);
	void auto_sequence_channels(int d);
	void process_simple_config(int d);

	void addUID(sAdvatekDevice* device);

	void refreshAdaptors();
	void setCurrentAdaptor(int adaptorIndex);
	
	void getJSON(sAdvatekDevice *fromDevice, sImportOptions &importOptions);
	void getJSON(sAdvatekDevice *device, boost::property_tree::ptree &JSONdevice);
	std::string importJSON(sAdvatekDevice *device, sImportOptions &importOptions);
	std::string importJSON(sAdvatekDevice *device, boost::property_tree::ptree advatek_device, sImportOptions &importOptions);
	void exportJSON(sAdvatekDevice *device, std::string path);
	void exportJSON(std::vector<sAdvatekDevice*> &devices, std::string path);
	std::string validateJSON(boost::property_tree::ptree advatek_devices);

	static const char* RGBW_Order[24];

	IClient* m_pUdpClient;

	bool bTestAll=false;
private:

};
