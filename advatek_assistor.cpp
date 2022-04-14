#include "advatek_assistor.h"
#include "udpclient.h"

#ifndef _WIN32
#include <netdb.h>
#include <ifaddrs.h>
#endif

namespace pt = boost::property_tree;

bool advatek_manager::deviceExist(uint8_t * Mac) {

	for (int d(0); d < connectedDevices.size(); d++) {
		bool exist = true;
		for (int i(0); i < 6; i++) {
			if (connectedDevices[d]->Mac[i] != Mac[i]) exist = false;
		}
		if (exist) return true;
	}

	return false;
}


void advatek_manager::listen() {
	uint8_t buffer[100000];

	if (m_pUdpClient && m_pUdpClient->HasMessages())
	{
		auto buf = m_pUdpClient->PopMessage();
		process_udp_message(buf.data());
	}
}

void advatek_manager::send_udp_message(std::string ip_address, int port, bool b_broadcast, std::vector<uint8_t> message)
{
	if (m_pUdpClient)
	{
		m_pUdpClient->Send(message, ip_address, b_broadcast);
	}

}

void advatek_manager::unicast_udp_message(std::string ip_address, std::vector<uint8_t> message)
{
	send_udp_message(ip_address, AdvPort, false, message);
}

void advatek_manager::broadcast_udp_message(std::vector<uint8_t> message)
{
	send_udp_message(AdvAdr, AdvPort, true, message);
}

void advatek_manager::identifyDevice(int d, uint8_t duration) {
	auto device = advatek_manager::connectedDevices[d];

	std::vector<uint8_t> dataTape;

	dataTape.resize(12);
	dataTape[0] = 'A';
	dataTape[1] = 'd';
	dataTape[2] = 'v';
	dataTape[3] = 'a';
	dataTape[4] = 't';
	dataTape[5] = 'e';
	dataTape[6] = 'c';
	dataTape[7] = 'h';
	dataTape[8] = 0x00;   // Null Terminator
	dataTape[9] = 0x00;   // OpCode
	dataTape[10] = 0x05;  // OpCode
	dataTape[11] = 0x08;  // ProtVer

	dataTape.insert(dataTape.end(), device->Mac, device->Mac + 6);
	dataTape.push_back(duration);

	unicast_udp_message(ipString(device->CurrentIP), dataTape);
}

void advatek_manager::copyToMemoryDevice(sAdvatekDevice* fromDevice) {
	memoryDevices.clear();
	sAdvatekDevice * memoryDevice = new sAdvatekDevice();
	copyDevice(fromDevice, memoryDevice);
	advatek_manager::memoryDevices.emplace_back(memoryDevice);
}

void advatek_manager::pasteFromMemoryDeviceTo(sAdvatekDevice* toDevice) {
	if (memoryDevices.size() == 0) {
		return;
	}
	sImportOptions importOptions = sImportOptions();
	importOptions.init = true;
	copyDevice(memoryDevices[0], toDevice);
}

void advatek_manager::addVirtualDevice(boost::property_tree::ptree advatek_device, sImportOptions &importOptions) {
	sAdvatekDevice * device = new sAdvatekDevice();
	importJSON(device, advatek_device, importOptions);
	advatek_manager::virtualDevices.emplace_back(device);
}

void advatek_manager::addVirtualDevice(sImportOptions &importOptions) {
	//if (importOptions.json.empty()) return;
	pt::ptree advatek_devices;
	pt::ptree advatek_device;

	std::stringstream ss;
	ss << importOptions.json;
	pt::read_json(ss, advatek_devices);
	
	if (advatek_devices.count("advatek_devices") > 0) {
		// Might have multiple devices
		for (auto &device : advatek_devices.get_child("advatek_devices"))
		{
			//advatek_device = device.second;
			addVirtualDevice(device.second, importOptions);
		}
	} else { // Single device
		//advatek_device = advatek_devices;
		addVirtualDevice(advatek_devices, importOptions);
	}
}

void advatek_manager::pasteToNewVirtualDevice() {
	sAdvatekDevice * device = new sAdvatekDevice();
	pasteFromMemoryDeviceTo(device);
	virtualDevices.emplace_back(device);
}

void advatek_manager::updateDevice(int d) {

	auto device = advatek_manager::connectedDevices[d];

	if ((bool)device->SimpleConfig) {
		advatek_manager::process_simple_config(d);
	}

	std::vector<uint8_t> dataTape;

	dataTape.resize(12);
	dataTape[0] = 'A';
	dataTape[1] = 'd';
	dataTape[2] = 'v';
	dataTape[3] = 'a';
	dataTape[4] = 't';
	dataTape[5] = 'e';
	dataTape[6] = 'c';
	dataTape[7] = 'h';
	dataTape[8] = 0x00;   // Null Terminator
	dataTape[9] = 0x00;   // OpCode
	dataTape[10] = 0x05;  // OpCode
	dataTape[11] = 0x08;  // ProtVer

	dataTape.insert(dataTape.end(), device->Mac, device->Mac + 6);

	dataTape.push_back(device->DHCP);

	dataTape.insert(dataTape.end(), device->StaticIP, device->StaticIP + 4);
	dataTape.insert(dataTape.end(), device->StaticSM, device->StaticSM + 4);

	dataTape.push_back(device->Protocol);
	dataTape.push_back(device->HoldLastFrame);
	dataTape.push_back(device->SimpleConfig);

	insertSwapped16(dataTape, device->OutputPixels, device->NumOutputs);
	insertSwapped16(dataTape, device->OutputUniv, device->NumOutputs);
	insertSwapped16(dataTape, device->OutputChan, device->NumOutputs);

	dataTape.insert(dataTape.end(), device->OutputNull, device->OutputNull + device->NumOutputs);
	insertSwapped16(dataTape, device->OutputZig, device->NumOutputs);

	dataTape.insert(dataTape.end(), device->OutputReverse, device->OutputReverse + device->NumOutputs);
	dataTape.insert(dataTape.end(), device->OutputColOrder, device->OutputColOrder + device->NumOutputs);
	insertSwapped16(dataTape, device->OutputGrouping, device->NumOutputs);

	dataTape.insert(dataTape.end(), device->OutputBrightness, device->OutputBrightness + device->NumOutputs);
	dataTape.insert(dataTape.end(), device->DmxOutOn, device->DmxOutOn + device->NumDMXOutputs);
	insertSwapped16(dataTape, device->DmxOutUniv, device->NumDMXOutputs);

	dataTape.push_back(device->CurrentDriver);
	dataTape.push_back(device->CurrentDriverType);

	dataTape.push_back(device->CurrentDriverSpeed);
	dataTape.push_back(device->CurrentDriverExpanded);

	dataTape.insert(dataTape.end(), device->Gamma, device->Gamma + 4);
	dataTape.insert(dataTape.end(), device->Nickname, device->Nickname + 40);

	dataTape.push_back(device->MaxTargetTemp);

	unicast_udp_message(ipString(device->CurrentIP), dataTape);
}

void advatek_manager::setTest(int d) {

	auto device = advatek_manager::connectedDevices[d];

	std::vector<uint8_t> dataTape;
	dataTape.resize(12);
	dataTape[0] = 'A';
	dataTape[1] = 'd';
	dataTape[2] = 'v';
	dataTape[3] = 'a';
	dataTape[4] = 't';
	dataTape[5] = 'e';
	dataTape[6] = 'c';
	dataTape[7] = 'h';
	dataTape[8] = 0x00;   // Null Terminator
	dataTape[9] = 0x00;   // OpCode
	dataTape[10] = 0x08;  // OpCode
	dataTape[11] = 0x08;  // ProtVer

	dataTape.insert(dataTape.end(), device->Mac, device->Mac + 6);

	dataTape.push_back(device->TestMode);

	dataTape.insert(dataTape.end(), device->TestCols, device->TestCols + 4);

	dataTape.push_back(device->TestOutputNum);

	dataTape.push_back((uint8_t)(device->TestPixelNum >> 8));
	dataTape.push_back((uint8_t)device->TestPixelNum);

	unicast_udp_message(ipString(device->CurrentIP), dataTape);
}

void advatek_manager::clearDevices(std::vector<sAdvatekDevice*> &devices) {
	for (auto device : devices)
	{
		if (device)
		{
			if (device->Model) delete device->Model;
			if (device->Firmware) delete device->Firmware;
			if (device->OutputPixels) delete[] device->OutputPixels;
			if (device->OutputUniv) delete[] device->OutputUniv;
			if (device->OutputChan) delete[] device->OutputChan;
			if (device->OutputNull) delete[] device->OutputNull;
			if (device->OutputZig) delete[] device->OutputZig;
			if (device->OutputReverse) delete[] device->OutputReverse;
			if (device->OutputColOrder) delete[] device->OutputColOrder;
			if (device->OutputGrouping) delete[] device->OutputGrouping;
			if (device->OutputBrightness) delete[] device->OutputBrightness;
			if (device->DmxOutOn) delete[] device->DmxOutOn;
			if (device->DmxOutUniv) delete[] device->DmxOutUniv;
			if (device->DriverType) delete[] device->DriverType;
			if (device->DriverSpeed) delete[] device->DriverSpeed;
			if (device->DriverExpandable) delete[] device->DriverExpandable;
			if (device->DriverNames) delete[] device->DriverNames;
			if (device->VoltageBanks) delete[] device->VoltageBanks;
			if (device) delete device;
		}
	}

	devices.clear();
}


void advatek_manager::clearConnectedDevices() {
	clearDevices(connectedDevices);
}

void advatek_manager::bc_networkConfig(int d) {
	auto device = advatek_manager::connectedDevices[d];
	std::vector<uint8_t> dataTape;
	dataTape.resize(12);
	dataTape[0] = 'A';
	dataTape[1] = 'd';
	dataTape[2] = 'v';
	dataTape[3] = 'a';
	dataTape[4] = 't';
	dataTape[5] = 'e';
	dataTape[6] = 'c';
	dataTape[7] = 'h';
	dataTape[8] = 0x00;   // Null Terminator
	dataTape[9] = 0x00;   // OpCode
	dataTape[10] = 0x07;  // OpCode
	dataTape[11] = 0x08;  // ProtVer

	dataTape.insert(dataTape.end(), device->Mac, device->Mac + 6);

	dataTape.push_back(device->DHCP);

	dataTape.insert(dataTape.end(), device->StaticIP, device->StaticIP + 4);
	dataTape.insert(dataTape.end(), device->StaticSM, device->StaticSM + 4);

	broadcast_udp_message(dataTape);
}

void advatek_manager::poll() {

	advatek_manager::clearDevices(connectedDevices);

	std::vector<uint8_t> dataTape;
	dataTape.resize(12);
	dataTape[0] = 'A';
	dataTape[1] = 'd';
	dataTape[2] = 'v';
	dataTape[3] = 'a';
	dataTape[4] = 't';
	dataTape[5] = 'e';
	dataTape[6] = 'c';
	dataTape[7] = 'h';
	dataTape[8] = 0x00;   // Null Terminator
	dataTape[9] = 0x00;   // OpCode
	dataTape[10] = 0x01;  // OpCode
	dataTape[11] = 0x08;  // ProtVer

	broadcast_udp_message(dataTape);

}

void advatek_manager::process_opPollReply(uint8_t * data) {
	sAdvatekDevice * rec_data = new sAdvatekDevice();

	memcpy(&rec_data->ProtVer, data, sizeof(uint8_t));
	data += 1;
	
	memcpy(&rec_data->CurrentProtVer, data, sizeof(uint8_t));
	data += 1;

	memcpy(rec_data->Mac, data, 6);
	data += 6;

	memcpy(&rec_data->ModelLength, data, sizeof(uint8_t));
	data += 1;

	rec_data->Model = new uint8_t[rec_data->ModelLength+1];
	memset(rec_data->Model, 0x00, sizeof(uint8_t) * (rec_data->ModelLength+1));
	memcpy(rec_data->Model, data, sizeof(uint8_t)*rec_data->ModelLength);
	data += rec_data->ModelLength;

	memcpy(&rec_data->HwRev, data, sizeof(uint8_t));
	data += 1;

	memcpy(rec_data->MinAssistantVer, data, sizeof(uint8_t) * 3);
	data += 3;

	memcpy(&rec_data->LenFirmware, data, sizeof(uint8_t));
	data += 1;

	rec_data->Firmware = new uint8_t[rec_data->LenFirmware + 1];
	memset(rec_data->Firmware, 0x00, sizeof(uint8_t) * (rec_data->LenFirmware + 1));
	memcpy(rec_data->Firmware, data, sizeof(uint8_t) * rec_data->LenFirmware);
	data += rec_data->LenFirmware;

	memcpy(&rec_data->Brand, data, sizeof(uint8_t));
	data += 1;

	memcpy(rec_data->CurrentIP, data, sizeof(uint8_t) * 4);
	data += 4;

	memcpy(rec_data->CurrentSM, data, sizeof(uint8_t) * 4);
	data += 4;

	memcpy(&rec_data->DHCP, data, sizeof(uint8_t));
	data += 1;

	memcpy(rec_data->StaticIP, data, sizeof(uint8_t) * 4);
	data += 4;

	memcpy(rec_data->StaticSM, data, sizeof(uint8_t) * 4);
	data += 4;

	memcpy(&rec_data->Protocol, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->HoldLastFrame, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->SimpleConfig, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->MaxPixPerOutput, data, sizeof(uint16_t));
	data += 2;
	bswap_16(rec_data->MaxPixPerOutput);

	memcpy(&rec_data->NumOutputs, data, sizeof(uint8_t));
	data += 1;
	rec_data->OutputPixels = new uint16_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputPixels, data, sizeof(uint16_t)*rec_data->NumOutputs);
	data += (rec_data->NumOutputs * 2);

	for (int i(0); i < (int)rec_data->NumOutputs; i++) {
		bswap_16(rec_data->OutputPixels[i]);
	}

	rec_data->OutputUniv = new uint16_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputUniv, data, sizeof(uint16_t)*rec_data->NumOutputs);
	data += (rec_data->NumOutputs * 2);

	for (int i(0); i < (int)rec_data->NumOutputs; i++) {
		bswap_16(rec_data->OutputUniv[i]);
	}

	rec_data->OutputChan = new uint16_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputChan, data, sizeof(uint16_t)*rec_data->NumOutputs);
	data += (rec_data->NumOutputs * 2);

	for (int i(0); i < (int)rec_data->NumOutputs; i++) {
		bswap_16(rec_data->OutputChan[i]);
	}

	rec_data->OutputNull = new uint8_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputNull, data, sizeof(uint8_t)*rec_data->NumOutputs);
	data += rec_data->NumOutputs;

	rec_data->OutputZig = new uint16_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputZig, data, sizeof(uint16_t)*rec_data->NumOutputs);
	data += (rec_data->NumOutputs * 2);

	for (int i(0); i < (int)rec_data->NumOutputs; i++) {
		bswap_16(rec_data->OutputZig[i]);
	}

	rec_data->OutputReverse = new uint8_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputReverse, data, sizeof(uint8_t)*rec_data->NumOutputs);
	data += rec_data->NumOutputs;

	rec_data->OutputColOrder = new uint8_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputColOrder, data, sizeof(uint8_t)*rec_data->NumOutputs);
	data += rec_data->NumOutputs;

	rec_data->OutputGrouping = new uint16_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputGrouping, data, sizeof(uint16_t)*rec_data->NumOutputs);
	data += (rec_data->NumOutputs * 2);

	for (int i(0); i < (int)rec_data->NumOutputs; i++) {
		bswap_16(rec_data->OutputGrouping[i]);
	}

	rec_data->OutputBrightness = new uint8_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputBrightness, data, sizeof(uint8_t)*rec_data->NumOutputs);
	data += rec_data->NumOutputs;

	memcpy(&rec_data->NumDMXOutputs, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->ProtocolsOnDmxOut, data, sizeof(uint8_t));
	data += 1;

	rec_data->DmxOutOn = new uint8_t[rec_data->NumDMXOutputs];
	memcpy(rec_data->DmxOutOn, data, sizeof(uint8_t)*rec_data->NumDMXOutputs);
	data += rec_data->NumDMXOutputs;

	rec_data->DmxOutUniv = new uint16_t[rec_data->NumDMXOutputs];
	memcpy(rec_data->DmxOutUniv, data, sizeof(uint16_t)*rec_data->NumDMXOutputs);
	data += (rec_data->NumDMXOutputs * 2);

	for (int i(0); i < (int)rec_data->NumDMXOutputs; i++) {
		bswap_16(rec_data->DmxOutUniv[i]);
	}

	memcpy(&rec_data->NumDrivers, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->DriverNameLength, data, sizeof(uint8_t));
	data += 1;

	rec_data->DriverType = new uint8_t[rec_data->NumDrivers];
	memcpy(rec_data->DriverType, data, sizeof(uint8_t)*rec_data->NumDrivers);
	data += rec_data->NumDrivers;

	rec_data->DriverSpeed = new uint8_t[rec_data->NumDrivers];
	memcpy(rec_data->DriverSpeed, data, sizeof(uint8_t)*rec_data->NumDrivers);
	data += rec_data->NumDrivers;

	rec_data->DriverExpandable = new uint8_t[rec_data->NumDrivers];
	memcpy(rec_data->DriverExpandable, data, sizeof(uint8_t)*rec_data->NumDrivers);
	data += rec_data->NumDrivers;

	rec_data->DriverNames = new char*[rec_data->NumDrivers];

	for (int i = 0; i < rec_data->NumDrivers; i++) {
		rec_data->DriverNames[i] = new char[rec_data->DriverNameLength + 1];
		memset(rec_data->DriverNames[i], 0, sizeof(uint8_t)*rec_data->DriverNameLength + 1);
		memcpy(rec_data->DriverNames[i], data, sizeof(uint8_t)*rec_data->DriverNameLength);
		data += rec_data->DriverNameLength;
	}

	memcpy(&rec_data->CurrentDriver, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->CurrentDriverType, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->CurrentDriverSpeed, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->CurrentDriverExpanded, data, sizeof(uint8_t));
	data += 1;

	memcpy(rec_data->Gamma, data, sizeof(uint8_t) * 4);
	data += 4;

	rec_data->Gammaf[0] = (float)rec_data->Gamma[0] * 0.1;
	rec_data->Gammaf[1] = (float)rec_data->Gamma[1] * 0.1;
	rec_data->Gammaf[2] = (float)rec_data->Gamma[2] * 0.1;
	rec_data->Gammaf[3] = (float)rec_data->Gamma[3] * 0.1;

	memcpy(rec_data->Nickname, data, sizeof(uint8_t) * 40);
	data += 40;

	memcpy(&rec_data->Temperature, data, sizeof(uint16_t));
	data += 2;
	bswap_16(rec_data->Temperature);

	memcpy(&rec_data->MaxTargetTemp, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->NumBanks, data, sizeof(uint8_t));
	data += 1;

	rec_data->VoltageBanks = new uint16_t[rec_data->NumBanks];
	memcpy(rec_data->VoltageBanks, data, sizeof(uint16_t)*rec_data->NumBanks);
	data += (rec_data->NumBanks * 2);

	for (int i(0); i < (int)rec_data->NumBanks; i++) {
		bswap_16(rec_data->VoltageBanks[i]);
	}

	memcpy(&rec_data->TestMode, data, sizeof(uint8_t));
	data += 1;

	memcpy(rec_data->TestCols, data, sizeof(uint8_t) * 4);
	data += 4;

	memcpy(&rec_data->TestOutputNum, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->TestPixelNum, data, sizeof(uint16_t));
	data += 2;
	bswap_16(rec_data->TestPixelNum);

	if (!deviceExist(rec_data->Mac)) advatek_manager::connectedDevices.emplace_back(rec_data);

}

void advatek_manager::process_opTestAnnounce(uint8_t * data) {

	uint8_t ProtVer;
	memcpy(&ProtVer, data, sizeof(uint8_t));
	data += 1;

	uint8_t Mac[6];
	memcpy( Mac, data, 6);
	data += 6;

	uint8_t CurrentIP[4];
	memcpy( CurrentIP, data, 4);
	data += 4;

	int deviceID = -1;

	for (int d(0); d < advatek_manager::connectedDevices.size(); d++) {
		bool exist = true;
		for (int i(0); i < 6; i++) {
			if (advatek_manager::connectedDevices[d]->Mac[i] != Mac[i]) exist = false;
		}
		if (exist) deviceID = d;
	}

	if (deviceID >= 0) {
		auto device = advatek_manager::connectedDevices[deviceID];

		memcpy(&device->TestMode, data, sizeof(uint8_t));
		data += 1;

		memcpy(device->TestCols, data, sizeof(uint8_t) * 4);
		data += 4;

		memcpy(&device->TestOutputNum, data, sizeof(uint8_t));
		data += 1;

		memcpy(&device->TestPixelNum, data, sizeof(uint16_t));
		data += 2;
		bswap_16(device->TestPixelNum);

	}

	return;
}

void advatek_manager::process_udp_message(uint8_t * data) {
	char ID[9];
	memcpy(ID, data, sizeof(uint8_t) * 9);
	data += 9;
	std::cout << "ID: " << ID << std::endl;

	std::string sid;
	for (int i(0); i < 8; i++) { sid += ID[i]; }
	if (sid.compare("Advatech") != 0) return; // Not an Advatek message ...

	//--------

	uint16_t OpCodes;
	memcpy(&OpCodes, data, sizeof(uint16_t));
	data += 2;
	// Swap bytes
	bswap_16(OpCodes);
	std::cout << "messageType: " << OpCodes << std::endl;

	switch (OpCodes) {
	case OpPollReply:
		process_opPollReply(data);
		return;
	case OpTestAnnounce:
		process_opTestAnnounce(data);
		return;
	default:
		return;
	}
}

void advatek_manager::auto_sequence_channels(int d) {
	auto device = advatek_manager::connectedDevices[d];

	uint16_t startOutputUniv   = device->OutputUniv[0];
	uint16_t startOutputChan   = device->OutputChan[0];
	uint16_t startOutputPixels = device->OutputPixels[0];

	uint16_t startEndUniverse = 0;
	uint16_t startEndChannel  = 0;

	setEndUniverseChannel(startOutputUniv, startOutputChan, startOutputPixels, device->OutputGrouping[0], startEndUniverse, startEndChannel);

	for (int output = 1; output < device->NumOutputs*0.5; output++) {
		if (startEndChannel + 1 > 510) {
			startEndUniverse = startEndUniverse + 1;
		}

		device->OutputUniv[output] = startEndUniverse;
		device->OutputChan[output] = (startEndChannel + 1) % 510;

		// Update loop
		startOutputUniv   = device->OutputUniv[output];
		startOutputChan   = device->OutputChan[output];
		startOutputPixels = device->OutputPixels[output];
		setEndUniverseChannel(startOutputUniv, startOutputChan, startOutputPixels, device->OutputGrouping[output], startEndUniverse, startEndChannel);

	}
	return;
}

void advatek_manager::process_simple_config(int d) {
	auto device = advatek_manager::connectedDevices[d];

	device->OutputNull[0] = 0;
	device->OutputZig[0] = 0;
	device->OutputGrouping[0] = 1;
	device->OutputBrightness[0] = 100;
	device->OutputReverse[0] = 0;

	for (int output = 1; output < device->NumOutputs*0.5; output++) {
		device->OutputNull[output] = device->OutputNull[0];
		device->OutputZig[output] = device->OutputZig[0];
		device->OutputGrouping[output] = device->OutputGrouping[0];
		device->OutputBrightness[output] = device->OutputBrightness[0];
		device->OutputReverse[output] = device->OutputReverse[0];

		device->OutputPixels[output]   = device->OutputPixels[0];
		device->OutputGrouping[output] = device->OutputGrouping[0];
	}
	advatek_manager::auto_sequence_channels(d);
	return;
}

void advatek_manager::refreshAdaptors() {

	networkAdaptors.clear();
	#ifndef _WIN32
	struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }


    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if((strcmp(ifa->ifa_name,"lo")!=0)&&(ifa->ifa_addr->sa_family==AF_INET))
        {
            if (s != 0)
            {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            printf("\tInterface : <%s>\n",ifa->ifa_name );
            printf("\t  Address : <%s>\n", host);
            networkAdaptors.push_back(std::string(host));
        }
    }

    freeifaddrs(ifaddr);
	    #else

	boost::asio::ip::tcp::resolver::iterator it;
	try
	{
		boost::asio::io_context io_context;
		boost::asio::ip::tcp::endpoint adaptorEndpoint(boost::asio::ip::address_v4::any(), AdvPort);

		boost::asio::ip::tcp::socket sock(io_context, adaptorEndpoint);

		boost::asio::ip::tcp::resolver resolver(io_context);

		boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(), "",boost::asio::ip::resolver_query_base::flags());
		it = resolver.resolve(query);
	}
	catch (boost::exception& e)
	{
		std::cout << "Query didn't resolve Any adapters - " << boost::diagnostic_information(e) << std::endl;
		return;
	}

	while (it != boost::asio::ip::tcp::resolver::iterator())
	{
		boost::asio::ip::address addr = (it++)->endpoint().address();
		std::cout << "adapter found: " << addr.to_string() << std::endl;
		if (addr.is_v4()) {
			networkAdaptors.push_back(addr.to_string());
		}
	}
	#endif

	if (!networkAdaptors.empty()) {
		currentAdaptor = 0;
		if (m_pUdpClient)
		{
			delete m_pUdpClient;
			m_pUdpClient = nullptr;
		}
		m_pUdpClient = new UdpClient(networkAdaptors[currentAdaptor].c_str(), AdvPort);
	} else {
		advatek_manager::currentAdaptor = -1;
	}

}

void advatek_manager::setCurrentAdaptor(int adaptorIndex ) {
	boost::system::error_code err;

	if (m_pUdpClient)
		delete m_pUdpClient;

	m_pUdpClient = new UdpClient(networkAdaptors[adaptorIndex].c_str(), AdvPort);
}

std::string advatek_manager::importJSON(sAdvatekDevice *device, boost::property_tree::ptree advatek_device, sImportOptions &importOptions) {
	std::string s_hold;
	std::stringstream report;

	if (importOptions.init) {
		SetValueFromJson(uint8_t, ProtVer);
		SetValueFromJson(uint8_t, CurrentProtVer);
		SetValueFromJson(uint8_t, ModelLength);
		//SetValueFromJson(uint8_t, LenFirmware);
		device->LenFirmware = 20;
		device->Model = new uint8_t[device->ModelLength + 1];
		memset(device->Model, 0x00, sizeof(uint8_t) * (device->ModelLength + 1));
		memcpy(device->Model, advatek_device.get<std::string>("Model").c_str(), sizeof(uint8_t) * device->ModelLength);
		s_hold = advatek_device.get<std::string>("Mac");
		load_macStr(s_hold, device->Mac);
	}

	if (advatek_device.get<std::string>("Model").compare(std::string((char *)device->Model)) == 0) {
		report << "Done!\n";
	}
	else {
		report << "Beware: Loaded data from ";
		report << advatek_device.get<std::string>("Model");
		report << " to ";
		report << device->Model;
		report << "\n";
	}

	if (importOptions.network || importOptions.init) {

		SetValueFromJson(uint8_t, DHCP);

		s_hold = advatek_device.get<std::string>("StaticIP");
		load_ipStr(s_hold, device->StaticIP);
		s_hold = advatek_device.get<std::string>("StaticSM");
		load_ipStr(s_hold, device->StaticSM);

		report << "- Import Network Settings Succesfull.\n";
	}

	if (importOptions.ethernet_control || importOptions.init) {
		bool go = EqualValueJson(uint8_t, NumOutputs);
		if (go || importOptions.init) {
			SetValueFromJson(uint8_t, HoldLastFrame);
			SetValueFromJson(uint8_t, SimpleConfig);
			SetValueFromJson(uint16_t, MaxPixPerOutput);
			SetValueFromJson(uint8_t, NumOutputs);
			if (importOptions.init) {
				device->OutputPixels = new uint16_t[device->NumOutputs];
				device->OutputUniv = new uint16_t[device->NumOutputs];
				device->OutputChan = new uint16_t[device->NumOutputs];
				device->OutputNull = new uint8_t[device->NumOutputs];
				device->OutputZig = new uint16_t[device->NumOutputs];
				device->OutputReverse = new uint8_t[device->NumOutputs];
				device->OutputColOrder = new uint8_t[device->NumOutputs];
				device->OutputGrouping = new uint16_t[device->NumOutputs];
				device->OutputBrightness = new uint8_t[device->NumOutputs];
			}
			SetChildIntValuesFromJson(OutputPixels);
			SetChildIntValuesFromJson(OutputUniv);
			SetChildIntValuesFromJson(OutputChan);
			SetChildIntValuesFromJson(OutputNull);
			SetChildIntValuesFromJson(OutputZig);
			SetChildIntValuesFromJson(OutputReverse);
			SetChildIntValuesFromJson(OutputColOrder);
			SetChildIntValuesFromJson(OutputGrouping);
			SetChildIntValuesFromJson(OutputBrightness);
			report << "- Import Ethernet Control Succesfull.\n";
		}
		else {
			report << "- Import Ethernet Control Failed. (Output count does not match)\n";
		}
	}

	if (importOptions.dmx_outputs || importOptions.init) {
		bool go = EqualValueJson(uint8_t, NumDMXOutputs);
		if (go || importOptions.init) {
			if (importOptions.init) {
				SetValueFromJson(uint8_t, NumDMXOutputs);
				device->DmxOutOn = new uint8_t[device->NumDMXOutputs];
				device->DmxOutUniv = new uint16_t[device->NumDMXOutputs];
			}
			SetValueFromJson(uint8_t, ProtocolsOnDmxOut);
			SetChildIntValuesFromJson(DmxOutOn);
			SetChildIntValuesFromJson(DmxOutUniv);
			report << "- Import DMX Control Succesfull.\n";
		}
		else {
			report << "- Import DMX Control Failed. (Output count does not match)\n";
		}
	}

	if (importOptions.led_settings || importOptions.init) {
		if (importOptions.init) {
			SetValueFromJson(uint8_t, NumDrivers);
			device->DriverType = new uint8_t[device->NumDrivers];
			device->DriverSpeed = new uint8_t[device->NumDrivers];
			device->DriverExpandable = new uint8_t[device->NumDrivers];
			device->DriverNames = new char*[device->NumDrivers];
			device->Firmware = new uint8_t[device->LenFirmware];
			memset(device->Firmware, 0, sizeof(uint8_t)*device->LenFirmware);
			char* tempFirmwareName = "Virtual";
			memcpy(device->Firmware, tempFirmwareName, 12);
			SetValueFromJson(uint8_t, DriverNameLength);
			for (int i = 0; i < device->NumDrivers; i++) {
				device->DriverNames[i] = new char[device->DriverNameLength + 1];
				memset(device->DriverNames[i], 0, sizeof(char) * (device->DriverNameLength + 1));
			}
			SetValueFromJson(uint8_t, DriverNameLength);
			for (pt::ptree::value_type &node : advatek_device.get_child("DriverNames"))
			{
				std::string sTempValue = node.second.data();
				int index = std::stoi(node.first);
				const char* sCStr = sTempValue.c_str();

				memcpy(device->DriverNames[index], sCStr, sizeof(char) * device->DriverNameLength);
			}
		}

		SetChildIntValuesFromJson(DriverType);
		SetChildIntValuesFromJson(DriverSpeed);
		SetChildIntValuesFromJson(DriverExpandable);

		SetValueFromJson(int, CurrentDriver);
		SetValueFromJson(uint8_t, CurrentDriverType);
		SetValueFromJson(int, CurrentDriverSpeed);
		SetValueFromJson(uint8_t, CurrentDriverExpanded);

		SetChildIntValuesFromJson(Gamma);
		device->Gammaf[0] = (float)device->Gamma[0] * 0.1;
		device->Gammaf[1] = (float)device->Gamma[1] * 0.1;
		device->Gammaf[2] = (float)device->Gamma[2] * 0.1;
		device->Gammaf[3] = (float)device->Gamma[3] * 0.1;

		report << "- Import LED Settings Succesfull.\n";
	}

	if (importOptions.nickname || importOptions.init) {
		s_hold = advatek_device.get<std::string>("Nickname");
		strncpy(device->Nickname, s_hold.c_str(), 40);
		report << "- Import Nickname Succesfull.\n";
	}

	if (importOptions.fan_on_temp) {
		SetValueFromJson(uint8_t, MaxTargetTemp);
		report << "- Import Fan On Temp Succesfull.\n";
	}

	return report.str();
}

std::string advatek_manager::importJSON(sAdvatekDevice *device, sImportOptions &importOptions) {
	pt::ptree advatek_devices;
	pt::ptree advatek_device;

	if (importOptions.json.empty()) {
		return "Error: No JSON data found.";
	} else { // data
		std::stringstream ss;
		ss << importOptions.json;
		pt::read_json(ss, advatek_devices);
	}

	if (advatek_devices.count("advatek_devices") > 0) {
		advatek_device = advatek_devices.get_child("advatek_devices").front().second;
	} else {
		advatek_device = advatek_devices;
	}

	return importJSON(device, advatek_device, importOptions);

}

void advatek_manager::getJSON(sAdvatekDevice *device, boost::property_tree::ptree &advatek_device) {

	advatek_device.put("ProtVer", device->ProtVer);
	advatek_device.put("CurrentProtVer", device->CurrentProtVer);
	advatek_device.put("Mac", macString(device->Mac));
	advatek_device.put("DHCP", device->DHCP);
	advatek_device.put("StaticIP", ipString(device->StaticIP));
	advatek_device.put("StaticSM", ipString(device->StaticSM));
	advatek_device.put("CurrentIP", ipString(device->CurrentIP));
	advatek_device.put("CurrentSM", ipString(device->CurrentSM));

	advatek_device.put("ModelLength", device->ModelLength);
	advatek_device.put("Model", device->Model);
	advatek_device.put("Brand", device->Brand);

	advatek_device.put("Protocol", device->Protocol);
	advatek_device.put("HoldLastFrame", device->HoldLastFrame);
	advatek_device.put("SimpleConfig", device->SimpleConfig);
	advatek_device.put("MaxPixPerOutput", device->MaxPixPerOutput);
	advatek_device.put("NumOutputs", device->NumOutputs);

	pt::ptree OutputPixels;
	pt::ptree OutputUniv;
	pt::ptree OutputChan;
	pt::ptree OutputNull;
	pt::ptree OutputZig;
	pt::ptree OutputReverse;
	pt::ptree OutputColOrder;
	pt::ptree OutputGrouping;
	pt::ptree OutputBrightness;

	for (int output = 0; output < device->NumOutputs; output++)
	{
		OutputPixels.put(std::to_string(output), std::to_string(device->OutputPixels[output]));
		OutputUniv.put(std::to_string(output), std::to_string(device->OutputUniv[output]));
		OutputChan.put(std::to_string(output), std::to_string(device->OutputChan[output]));
		OutputNull.put(std::to_string(output), std::to_string(device->OutputNull[output]));
		OutputZig.put(std::to_string(output), std::to_string(device->OutputZig[output]));
		OutputReverse.put(std::to_string(output), std::to_string(device->OutputReverse[output]));
		OutputColOrder.put(std::to_string(output), std::to_string(device->OutputColOrder[output]));
		OutputGrouping.put(std::to_string(output), std::to_string(device->OutputGrouping[output]));
		OutputBrightness.put(std::to_string(output), std::to_string(device->OutputBrightness[output]));
	}

	advatek_device.add_child("OutputPixels", OutputPixels);
	advatek_device.add_child("OutputUniv", OutputUniv);
	advatek_device.add_child("OutputChan", OutputChan);
	advatek_device.add_child("OutputNull", OutputNull);
	advatek_device.add_child("OutputZig", OutputZig);
	advatek_device.add_child("OutputReverse", OutputReverse);
	advatek_device.add_child("OutputColOrder", OutputColOrder);
	advatek_device.add_child("OutputGrouping", OutputGrouping);
	advatek_device.add_child("OutputBrightness", OutputBrightness);

	advatek_device.put("NumDMXOutputs", device->NumDMXOutputs);
	advatek_device.put("ProtocolsOnDmxOut", device->ProtocolsOnDmxOut);

	pt::ptree DmxOutOn;
	pt::ptree DmxOutUniv;

	for (int output = 0; output < device->NumDMXOutputs; output++)
	{
		DmxOutOn.put(std::to_string(output), std::to_string(device->DmxOutOn[output]));
		DmxOutUniv.put(std::to_string(output), std::to_string(device->DmxOutUniv[output]));
	}

	advatek_device.add_child("DmxOutOn", DmxOutOn);
	advatek_device.add_child("DmxOutUniv", DmxOutUniv);

	advatek_device.put("NumDrivers", device->NumDrivers);
	advatek_device.put("DriverNameLength", device->DriverNameLength);

	pt::ptree DriverType;
	pt::ptree DriverSpeed;
	pt::ptree DriverExpandable;
	pt::ptree DriverNames;

	for (int output = 0; output < device->NumDrivers; output++)
	{
		DriverType.put(std::to_string(output), std::to_string(device->DriverType[output]));
		DriverSpeed.put(std::to_string(output), std::to_string(device->DriverSpeed[output]));
		DriverExpandable.put(std::to_string(output), std::to_string(device->DriverExpandable[output]));
		DriverNames.put(std::to_string(output), (device->DriverNames[output]));
	}

	advatek_device.add_child("DriverType", DriverType);
	advatek_device.add_child("DriverSpeed", DriverSpeed);
	advatek_device.add_child("DriverExpandable", DriverExpandable);
	advatek_device.add_child("DriverNames", DriverNames);

	advatek_device.put("CurrentDriver", device->CurrentDriver);
	advatek_device.put("CurrentDriverType", device->CurrentDriverType);
	advatek_device.put("CurrentDriverSpeed", device->CurrentDriverSpeed);
	advatek_device.put("CurrentDriverExpanded", device->CurrentDriverExpanded);

	pt::ptree Gamma;

	for (int c = 0; c < 4; c++)
	{
		Gamma.put(std::to_string(c), std::to_string(device->Gamma[c]));
	}

	advatek_device.add_child("Gamma", Gamma);

	advatek_device.put("Nickname", device->Nickname);
	advatek_device.put("MaxTargetTemp", device->MaxTargetTemp);
	advatek_device.put("NumBanks", device->NumBanks);

}

void advatek_manager::getJSON(sAdvatekDevice *fromDevice, sImportOptions &importOptions) {
	pt::ptree data;
	getJSON(fromDevice, data);

	std::stringstream jsonStringStream;
	write_json(jsonStringStream, data);

	importOptions.json = jsonStringStream.str();
}

void advatek_manager::exportJSON(std::vector<sAdvatekDevice*> &devices, std::string path) {
	pt::ptree advatek_devices;
	pt::ptree devicesArr;

	for (auto & device : devices) {
		pt::ptree advatek_device;
		getJSON(device, advatek_device);
		devicesArr.push_back(std::make_pair("", advatek_device));
	}

	advatek_devices.add_child("advatek_devices", devicesArr);

	std::ofstream outfile;
	outfile.open(path, std::ios::out | std::ios::trunc);
	pt::write_json(outfile, advatek_devices);
	outfile.close();
}

void advatek_manager::exportJSON(sAdvatekDevice *device, std::string path) {
	
	pt::ptree advatek_devices;
	pt::ptree devices;
	pt::ptree advatek_device;

	getJSON(device, advatek_device);

	devices.push_back(std::make_pair("", advatek_device));
	advatek_devices.add_child("advatek_devices", devices);

	//pt::write_json(std::cout, advatek_devices);

	std::ofstream outfile;
	outfile.open(path, std::ios::out | std::ios::trunc);
	//outfile << pt::write_json(std::cout, data) << endl;
	pt::write_json(outfile, advatek_devices);
	outfile.close();
}

void advatek_manager::copyDevice(sAdvatekDevice *fromDevice, sAdvatekDevice *toDevice) {
	pt::ptree advatek_device;
	getJSON(fromDevice, advatek_device);

	std::stringstream jsonStringStream;
	write_json(jsonStringStream, advatek_device);

	sImportOptions importOptions = sImportOptions();
	importOptions.json = jsonStringStream.str();
	importOptions.init = true;

	importJSON(toDevice, importOptions);
}
