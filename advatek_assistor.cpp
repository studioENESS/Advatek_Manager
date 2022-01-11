#include "advatek_assistor.h"

std::vector<sAdvatekDevice*> devices;
std::vector <std::string> networkAdaptors;

const char* RGBW_Order[24] = {
		"R-G-B/R-G-B-W",
		"R-B-G/R-B-G-W",
		"G-R-B/G-R-B-W",
		"B-R-G/B-R-G-W",
		"G-B-R/G-B-R-W",
		"B-G-R/B-G-R-W",
		"R-G-W-B",
		"R-B-W-G",
		"G-R-W-B",
		"B-R-W-G",
		"G-B-W-R",
		"B-G-W-R",
		"R-W-G-B",
		"R-W-B-G",
		"G-W-R-B",
		"B-W-R-G",
		"G-W-B-R",
		"B-W-G-R",
		"W-R-G-B",
		"W-R-B-G",
		"W-G-R-B",
		"W-B-R-G",
		"W-G-B-R",
		"W-B-G-R"
};

const char* DriverTypes[3] = {
		"RGB only",
		"RGBW only",
		"Either RGB or RGBW"
};

const char* DriverSpeedsMhz[12] = {
	"0.4 MHz", // Data Only Slow
	"0.6 MHz", // Data Only Fast
	"0.8 MHz",
	"1.0 MHz",
	"1.2 MHz",
	"1.4 MHz",
	"1.6 MHz",
	"1.8 MHz",
	"2.0 MHz",
	"2.2 MHz",
	"2.5 MHz",
	"2.9 MHz"
};

const char* TestModes[9] = {
	"None(Live Control Data)",
	"RGBW Cycle",
	"Red",
	"Green",
	"Blue",
	"White",
	"Set Color",
	"Color Fade",
	"Single Pixel"
};

std::string macString(uint8_t * address) {
	std::stringstream ss;

	for (int i(0); i < 6; i++) {
		ss << std::hex << std::setw(2) << static_cast<int>(address[i]);
		if (i < 5) ss << ":";
	}

	return ss.str();
}

std::string ipString(uint8_t * address) {
	std::stringstream ss;

	for (int i(0); i < 4; i++) {
		ss << static_cast<int>(address[i]);
		if (i < 3) ss << ".";
	}

	return ss.str();
}

void insertSwapped16(std::vector<uint8_t> &dest, uint16_t* pData, int32_t size)
{
	for (int i = 0; i < size; i++)
	{
		uint16_t data = pData[i];
		dest.push_back((uint8_t)(data >> 8));
		dest.push_back((uint8_t)data);
	}
}

bool deviceExist(uint8_t * Mac) {

	for (int d(0); d < devices.size(); d++) {
		bool exist = true;
		for (int i(0); i < 6; i++) {
			if (devices[d]->Mac[i] != Mac[i]) exist = false;
		}
		if (exist) return true;
	}

	return false;
}

boost::asio::io_context io_context;
boost::asio::ip::udp::endpoint receiver(boost::asio::ip::udp::v4(), AdvPort);
boost::asio::ip::udp::socket s_socket(io_context);
boost::asio::ip::udp::socket r_socket = boost::asio::ip::udp::socket(io_context, receiver);

boost::asio::ip::tcp::resolver resolver(io_context);
boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(), "");

void send_udp_message(std::string ip_address, int port, bool b_broadcast, std::vector<uint8_t> message)
{
	s_socket.open(boost::asio::ip::udp::v4());
	s_socket.set_option(boost::asio::socket_base::broadcast(b_broadcast));

	boost::asio::ip::udp::endpoint senderEndpoint(boost::asio::ip::address::from_string(ip_address), port);

	try
	{
		s_socket.send_to(boost::asio::buffer(message), senderEndpoint);
		s_socket.close();
		printf("Message sent to %s\n", ip_address.c_str());

	}
	catch (std::exception e)
	{
		printf("Cannot broadcast message to %s \n", ip_address.c_str());
	}
}

void unicast_udp_message(std::string ip_address, std::vector<uint8_t> message)
{
	send_udp_message(ip_address, AdvPort, false, message);
}

void broadcast_udp_message(std::vector<uint8_t> message)
{
	send_udp_message(AdvAdr, AdvPort, true, message);
}

void updateDevice(int d) {
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

	dataTape.insert(dataTape.end(), devices[d]->Mac, devices[d]->Mac + 6);

	dataTape.push_back(devices[d]->DHCP);

	dataTape.insert(dataTape.end(), devices[d]->StaticIP, devices[d]->StaticIP + 4);
	dataTape.insert(dataTape.end(), devices[d]->StaticSM, devices[d]->StaticSM + 4);

	dataTape.push_back(devices[d]->Protocol);
	dataTape.push_back(devices[d]->HoldLastFrame);
	dataTape.push_back(devices[d]->SimpleConfig);

	insertSwapped16(dataTape, devices[d]->OutputPixels, devices[d]->NumOutputs);
	insertSwapped16(dataTape, devices[d]->OutputUniv, devices[d]->NumOutputs);
	insertSwapped16(dataTape, devices[d]->OutputChan, devices[d]->NumOutputs);

	dataTape.insert(dataTape.end(), devices[d]->OutputNull, devices[d]->OutputNull + devices[d]->NumOutputs);
	insertSwapped16(dataTape, devices[d]->OutputZig, devices[d]->NumOutputs);

	dataTape.insert(dataTape.end(), devices[d]->OutputReverse, devices[d]->OutputReverse + devices[d]->NumOutputs);
	dataTape.insert(dataTape.end(), devices[d]->OutputColOrder, devices[d]->OutputColOrder + devices[d]->NumOutputs);
	insertSwapped16(dataTape, devices[d]->OutputGrouping, devices[d]->NumOutputs);

	dataTape.insert(dataTape.end(), devices[d]->OutputBrightness, devices[d]->OutputBrightness + devices[d]->NumOutputs);
	dataTape.insert(dataTape.end(), devices[d]->DmxOutOn, devices[d]->DmxOutOn + devices[d]->NumDMXOutputs);
	insertSwapped16(dataTape, devices[d]->DmxOutUniv, devices[d]->NumDMXOutputs);

	dataTape.push_back(devices[d]->CurrentDriver);
	dataTape.push_back(devices[d]->CurrentDriverType);

	dataTape.push_back(devices[d]->CurrentDriverSpeed);
	dataTape.push_back(devices[d]->CurrentDriverExpanded);

	dataTape.insert(dataTape.end(), devices[d]->Gamma, devices[d]->Gamma + 4);
	dataTape.insert(dataTape.end(), devices[d]->Nickname, devices[d]->Nickname + 40);

	dataTape.push_back(devices[d]->MaxTargetTemp);

	unicast_udp_message(ipString(devices[d]->CurrentIP), dataTape);
}

void setTest(int d) {

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

	dataTape.insert(dataTape.end(), devices[d]->Mac, devices[d]->Mac + 6);

	dataTape.push_back(devices[d]->TestMode);

	dataTape.insert(dataTape.end(), devices[d]->TestCols, devices[d]->TestCols + 4);

	dataTape.push_back(devices[d]->TestOutputNum);

	dataTape.push_back((uint8_t)(devices[d]->TestPixelNum >> 8));
	dataTape.push_back((uint8_t)devices[d]->TestPixelNum);

	//buf[23] = 0;// devices[d]->TestOutputNum;
	//buf[24] = 0;//devices[d]->TestPixelNum;
	//buf[25] = 0;//devices[d]->TestPixelNum >> 8;

	unicast_udp_message(ipString(devices[d]->CurrentIP), dataTape);
}

void clearDevices() {
	for (auto device : devices)
	{
		if (device)
		{
			delete device->Model;
			delete device->Firmware;
			delete[] device->OutputPixels;
			delete[] device->OutputUniv;
			delete[] device->OutputChan;
			delete[] device->OutputNull;
			delete[] device->OutputZig;
			delete[] device->OutputReverse;
			delete[] device->OutputColOrder;
			delete[] device->OutputGrouping;
			delete[] device->OutputBrightness;
			delete[] device->DmxOutOn;
			delete[] device->DmxOutUniv;
			delete[] device->DriverType;
			delete[] device->DriverSpeed;
			delete[] device->DriverExpandable;
			delete[] device->DriverNames;
			delete[] device->VoltageBanks;
			delete device;
		}
	}

	devices.clear();
}

void setEndUniverseChannel(int startUniverse, int startChannel, int pixelCount, int outputGrouping, int &endUniverse, int &endChannel) {
	pixelCount *= outputGrouping;
	int pixelChannels = (3 * pixelCount); // R-G-B data
	int pixelUniverses = ((float)(startChannel + pixelChannels) / 510.f);

	endUniverse = startUniverse + pixelUniverses;
	endChannel = (startChannel + pixelChannels - 1) % 510;
}

void bc_networkConfig(int d) {
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

	dataTape.insert(dataTape.end(), devices[d]->Mac, devices[d]->Mac + 6);

	dataTape.push_back(devices[d]->DHCP);

	dataTape.insert(dataTape.end(), devices[d]->StaticIP, devices[d]->StaticIP + 4);
	dataTape.insert(dataTape.end(), devices[d]->StaticSM, devices[d]->StaticSM + 4);

	broadcast_udp_message(dataTape);
}

void poll() {

	clearDevices();

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

void process_opPollReply(uint8_t * data) {
	sAdvatekDevice * rec_data = new sAdvatekDevice();

	memcpy(&rec_data->ProtVer, data, sizeof(uint8_t));
	data += 1;
	//std::cout << "ProtVer: " << (int)rec_data->ProtVer << std::endl;

	memcpy(&rec_data->CurrentProtVer, data, sizeof(uint8_t));
	data += 1;

	memcpy(rec_data->Mac, data, 6);
	data += 6;

	memcpy(&rec_data->ModelLength, data, sizeof(uint8_t));
	data += 1;

	rec_data->Model = new uint8_t[rec_data->ModelLength];
	memcpy(rec_data->Model, data, sizeof(uint8_t)*rec_data->ModelLength);
	data += rec_data->ModelLength;

	memcpy(&rec_data->HwRev, data, sizeof(uint8_t));
	data += 1;

	memcpy(rec_data->MinAssistantVer, data, sizeof(uint8_t) * 3);
	data += 3;

	memcpy(&rec_data->LenFirmware, data, sizeof(uint8_t));
	data += 1;

	rec_data->Firmware = new uint8_t[rec_data->LenFirmware];
	memcpy(rec_data->Firmware, data, sizeof(uint8_t)*rec_data->LenFirmware);
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

	if (!deviceExist(rec_data->Mac)) devices.emplace_back(rec_data);

}

void process_opTestAnnounce(uint8_t * data) {
	return;
}

void process_udp_message(uint8_t * data) {
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

void refreshAdaptors() {

	networkAdaptors.clear();
	boost::asio::ip::tcp::resolver::iterator it;
	try
	{
		it = resolver.resolve(query);
	}
	catch (boost::exception& e)
	{
		std::cout << "Query didn't resolve Any adaptors" << std::endl;
	}

	while (it != boost::asio::ip::tcp::resolver::iterator())
	{
		boost::asio::ip::address addr = (it++)->endpoint().address();
		std::cout << "adaptor found: " << addr.to_string() << std::endl;
		if (addr.is_v4()) {
			networkAdaptors.push_back(addr.to_string());
		}
	}
	if (networkAdaptors.size() > 0) {
		adaptor_string = networkAdaptors[0];
	}
}

std::string advatek_manager::macStr(uint8_t * address)
{
	std::stringstream ss;

	for (int i(0); i < 6; i++) {
		ss << std::hex << std::setw(2) << static_cast<int>(address[i]);
		if (i < 5) ss << ":";
	}

	return ss.str();
}

std::string advatek_manager::ipStr(uint8_t * address)
{
	std::stringstream ss;

	for (int i(0); i < 4; i++) {
		ss << static_cast<int>(address[i]);
		if (i < 3) ss << ".";
	}

	return ss.str();
}
