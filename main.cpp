// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#define AdvAdr "255.255.255.255" // Advatek zero network broadcast address
#define AdvPort 49150 // Advatek UDP Port
#define OpPoll	0x0001 // A request for controllers to identify themselves.
#define OpPollReply 0x0002 //Reply from a controller to a poll(includes configuration data).
#define OpConfig  0x0005 //New configuration data being directed at a controller.
#define OpBootload 0x0006 // Send the controller into bootloader mode.
#define OpNetwork 0x0007 // Change network settings only.
#define OpTestSet 0x0008 // Set the test mode.
#define OpTestAnnounce 0x0009 // Announce the current test mode.
#define OpVisualIdent 0x000a // A broadcast to request the controller of the designated MAC to identify its location by flashing its status LED.

#define bswap_16(x) x=((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))

bool b_refreshAdaptorsRequest = false;
int currentAdaptor = 0;
bool b_pollRequest = false;
int id_networkConfigRequest = -1;

std::vector <std::string> networkAdaptors;
static std::string adaptor_string = "Select Adaptor";

typedef struct sAdvatekDevice {
	uint8_t ProtVer; // Protocol version
	uint8_t CurrentProtVer; // Using Protocol version
	uint8_t Mac[6]; // MAC Address
	uint8_t ModelLength; // Length of model array
	uint8_t* Model; // Full display name
	uint8_t HwRev; // HW rev E.G. 1.0 = 10
	uint8_t MinAssistantVer[3]; // Minimum Advatek Assistant version required for config
	uint8_t LenFirmware;
	uint8_t* Firmware; // firmware version, null terminated
	uint8_t Brand; // 0 = Advatek
	uint8_t CurrentIP[4]; // IP address
	uint8_t CurrentSM[4]; // Current Subnet Mask (incase DHCP issued)
	uint8_t DHCP; // DHCP on or off
	uint8_t StaticIP[4]; // Static IP address
	uint8_t StaticSM[4]; // Static subnet mask

	uint8_t Protocol; // 0 = sACN, 1 = artNet
	uint8_t HoldLastFrame; // Hold last frame (timeout blank mode)
	uint8_t SimpleConfig; // Simple = 0, or advanced config = 1
	uint16_t MaxPixPerOutput; // Maximum number of pixels any output can drive
	uint8_t NumOutputs; // Number of outputs (including expanded)

	// NumOutputs
	uint16_t* OutputPixels;//[2 * NumOutputs]; // Number of nodes per string
	uint16_t* OutputUniv;//[2 * NumOutputs]; // Advanced start universes
	uint16_t* OutputChan;//[2 * NumOutputs]; // Advanced start channels
	uint8_t* OutputNull;//[NumOutputs]; // Null pixels
	uint16_t* OutputZig;//[2 * NumOutputs]; // Zig zags
	uint8_t* OutputReverse;//[NumOutputs]; // Reversed strings
	uint8_t* OutputColOrder;//[NumOutputs]; // RGB order for each output
	uint16_t* OutputGrouping;//[2 * NumOutputs]; // Pixel grouping
	uint8_t* OutputBrightness;//[NumOutputs]; // Brightness limiting
	uint8_t NumDMXOutputs; // Number of DMX outputs
	uint8_t ProtocolsOnDmxOut;
	uint8_t* DmxOutOn;//[NumDMXOutputs]; // DMX outputs on or off
	uint16_t* DmxOutUniv;//[2 * NumDMXOutputs]; // Hi and Lo bytes of DMX output universes
	uint8_t NumDrivers; // Number of pixel drivers
	uint8_t DriverNameLength; // Length of pixel driver strings
	uint8_t* DriverType;//[NumDrivers]; // 0 = RGB only, 1 = RGBW only, 2 = Either
	uint8_t* DriverSpeed;//[NumDrivers]; // 0 = N/A, 1 = slow only, 2 = fast only, 3 = either, 4 = adjustable clock 0.4 - 2.9MHz(12 step)
	uint8_t* DriverExpandable;// [NumDrivers]; // 0 = Normal mode only, 1 = capable of expanded mode
	char** DriverNames;// [NumDrivers][LENGTH_DRIVER_STRINGS]; // Null terminated strings of driver types
	int CurrentDriver; // Current pixel protocol selection (index)
	uint8_t CurrentDriverType; // RGB = 0, RGBW = 1
	int CurrentDriverSpeed; // Output chip speed (0 = Slow, 1 = Fast)
	uint8_t CurrentDriverExpanded; // Expanded/Condensed Mode
	uint8_t Gamma[4]; // R, G & B Gamma
	float Gammaf[4];
	char Nickname[40]; // if the product has a nickname, null terminatedDriverNames    uint16_t Temperature; // Hi byte of temp reading
	uint16_t Temperature;
	uint8_t MaxTargetTemp; // Max target temperature (fan control). 0xFF means no fan control.
	uint8_t NumBanks; // Number of banks for voltage readings
	uint16_t* VoltageBanks;// [NumBanks][2]; // Voltage on power banks (*10)
	int TestMode; // Current test mode program (0 = off/live data)
	int TestCols[4];
	uint8_t TestOutputNum;
	uint16_t TestPixelNum;
} sAdvatekDevice;

std::vector<sAdvatekDevice*> devices;

std::string macString(uint8_t * address) {
	std::stringstream ss;

	for (int i(0); i < 6; i++) {
		ss << std::hex << std::setw(2) << static_cast<int>(address[i]);
		if (i < 5) ss << ":";
	}

	return ss.str();
}

std::string addressString(uint8_t * address) {
	std::stringstream ss;

	for (int i(0); i < 4; i++) {
		ss << static_cast<int>(address[i]);
		if (i < 3) ss << ".";
	}

	return ss.str();
}

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

const char* DriverSpeeds[5] = {
	"N/A (single fixed speed)",
	"Slow only (data only driver, fixed at the slow speed)",
	"Fast only (data only driver, fixed at the fast speed)",
	"Either slow or fast (selectable between 2 speeds)",
	"Adjustable clock speed"
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

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
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
//boost::asio::ip::udp::endpoint receiver = boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), AdvPort);

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

	unicast_udp_message(addressString(devices[d]->CurrentIP), dataTape);
}

void setTest(int d)  {

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

	unicast_udp_message(addressString(devices[d]->CurrentIP), dataTape);
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

void process_opPollReply(uint8_t * data ) {
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

void process_udp_message( uint8_t * data ) {
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
			process_opPollReply( data );
			return;
		case OpTestAnnounce:
			process_opTestAnnounce( data );
			return;
		default:
			return;
	}
}

void refreshAdaptors() {

	networkAdaptors.clear();

	boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query);

	while (it != boost::asio::ip::tcp::resolver::iterator())
	{
		boost::asio::ip::address addr = (it++)->endpoint().address();
		std::cout << "adaptor found: " << addr.to_string() << std::endl;
		if (addr.is_v4()) {
			networkAdaptors.push_back(addr.to_string());
		}
	}
	if (networkAdaptors.size() > 0 ) {
		adaptor_string = networkAdaptors[0];
	}
}

int main(int, char**)
{

	refreshAdaptors();

	poll();

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	int window_w = 800;
	int window_h = 600;
    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(window_w, window_h, "Advatek Assistant", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
		uint8_t buffer[100000];
		
		if (r_socket.available() > 0)
		{
			std::size_t bytes_transferred = r_socket.receive_from(boost::asio::buffer(buffer), receiver);
			if (bytes_transferred > 1) {  // we have data
				process_udp_message(buffer);
			}
		}

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            // Create a window and append into it.
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(ImVec2(window_w, window_h));

			ImGuiWindowFlags window_flags = 0;
			window_flags |= ImGuiWindowFlags_NoTitleBar;
			window_flags |= ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoResize;
			window_flags |= ImGuiWindowFlags_NoCollapse;
			ImGui::Begin("Advatek Assistant", NULL, window_flags);                    

			if (ImGui::Button("Refresh Adaptors"))
			{
				b_refreshAdaptorsRequest = true;
			} ImGui::SameLine();

			ImGui::PushItemWidth(120);

			if (ImGui::BeginCombo("###Adaptor", adaptor_string.c_str(), 0))
			{
				for (int n = 0; n < networkAdaptors.size(); n++)
				{
					const bool is_selected = (currentAdaptor == n);
					if (ImGui::Selectable(networkAdaptors[n].c_str(), is_selected))
					{
						currentAdaptor = n;
						adaptor_string = networkAdaptors[n];
						r_socket.close();
						//r_socket.open(boost::asio::ip::udp::v4());
						//r_socket.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(adaptor_string), AdvPort));

						//receiver = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(adaptor_string), AdvPort);
						receiver = boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), AdvPort);
						r_socket = boost::asio::ip::udp::socket(io_context, receiver);

						b_pollRequest = true;
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
				ImGui::PopItemWidth();
			}


			if (ImGui::Button("Search"))
			{
				b_pollRequest = true;
			} ImGui::SameLine();

			ImGui::Text("%i Device(s) Connected", devices.size());

			for (uint8_t i = 0; i < devices.size(); i++) {
				std::stringstream Title;
				Title << devices[i]->Model << "	" << devices[i]->Firmware << "	" << addressString(devices[i]->CurrentIP) << "		" << "Temp: " << (float)devices[i]->Temperature*0.1 << "		" << devices[i]->Nickname;
				Title << "###" << macString(devices[i]->Mac);
				bool node_open = ImGui::TreeNodeEx(Title.str().c_str(), ImGuiSelectableFlags_SpanAllColumns | ImGuiTreeNodeFlags_OpenOnArrow);

				if (node_open)
				{
					ImGui::Columns(1);
					ImGui::BeginTabBar("MyTabBar", ImGuiTabBarFlags_None);
					if (ImGui::BeginTabItem("Network"))
					{
						ImGui::Text("Static IP Address:");

						ImGui::PushItemWidth(30);

						ImGui::InputScalar(".##CurrentIP0", ImGuiDataType_U8, &devices[i]->StaticIP[0], 0, 0, 0);

						ImGui::SameLine();
						ImGui::InputScalar(".##CurrentIP1", ImGuiDataType_U8, &devices[i]->StaticIP[1], 0, 0, 0);

						ImGui::SameLine();
						ImGui::InputScalar(".##CurrentIP2", ImGuiDataType_U8, &devices[i]->StaticIP[2], 0, 0, 0);

						ImGui::SameLine();
						ImGui::InputScalar("##CurrentIP3", ImGuiDataType_U8, &devices[i]->StaticIP[3], 0, 0, 0);


						ImGui::Text("Static Subnet Mask:");

						ImGui::InputScalar(".##CurrentSM0", ImGuiDataType_U8, &devices[i]->StaticSM[0], 0, 0, 0);

						ImGui::SameLine();
						ImGui::InputScalar(".##CurrentSM1", ImGuiDataType_U8, &devices[i]->StaticSM[1], 0, 0, 0);

						ImGui::SameLine();
						ImGui::InputScalar(".##CurrentSM2", ImGuiDataType_U8, &devices[i]->StaticSM[2], 0, 0, 0);

						ImGui::SameLine();
						ImGui::InputScalar("##CurrentSM3", ImGuiDataType_U8, &devices[i]->StaticSM[3], 0, 0, 0);

						ImGui::PopItemWidth();

						ImGui::Text("IP Type: "); ImGui::SameLine();
						int tempDHCP = (int)devices[i]->DHCP;
						if (ImGui::RadioButton("DHCP", &tempDHCP, 1)) {
							devices[i]->DHCP = 1;
						} ImGui::SameLine();
						if (ImGui::RadioButton("Static", &tempDHCP, 0)) {
							devices[i]->DHCP = 0;
						}

						if (ImGui::Button("Update Network Config"))
						{
							id_networkConfigRequest = i;
						}

						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Control"))
					{
						ImGui::Text("Ethernet Control: "); ImGui::SameLine();
						int tempProtocol = (int)devices[i]->Protocol;
						if (ImGui::RadioButton("ArtNet", &tempProtocol, 1)) {
							devices[i]->Protocol = 1;
						} ImGui::SameLine();
						if (ImGui::RadioButton("sACN (E1.31)", &tempProtocol, 0)) {
							devices[i]->Protocol = 0;
						}
						ImGui::SameLine();
						static bool tempHoldLastFrame = (bool)devices[i]->HoldLastFrame;
						if (ImGui::Checkbox("Hold LastFrame", &tempHoldLastFrame)) {
							devices[i]->HoldLastFrame = tempHoldLastFrame;
						}

						bool * tempReversed = new bool[devices[i]->NumOutputs];
						int  * tempEndUniverse = new int[devices[i]->NumOutputs];
						int  * tempEndChannel = new int[devices[i]->NumOutputs];

						ImGui::PushItemWidth(40);

						for (uint8_t output = 0; output < devices[i]->NumOutputs*0.5; output++) {

							setEndUniverseChannel((int)devices[i]->OutputUniv[output], (int)devices[i]->OutputChan[output], (int)devices[i]->OutputPixels[output], (int)devices[i]->OutputGrouping[output], tempEndUniverse[output], tempEndChannel[output]);

							ImGui::PushID(output);

							ImGui::Text("Output %i", output + 1); ImGui::SameLine();
							ImGui::InputScalar("##StartUniv", ImGuiDataType_U16, &devices[i]->OutputUniv[output], 0, 0, 0); ImGui::SameLine();
							ImGui::InputScalar("##StartChan", ImGuiDataType_U16, &devices[i]->OutputChan[output], 0, 0, 0); ImGui::SameLine();
							ImGui::InputScalar("##EndUniv", ImGuiDataType_U16, &tempEndUniverse[output], 0, 0, 0); ImGui::SameLine();
							ImGui::InputScalar("##EndChan", ImGuiDataType_U16, &tempEndChannel[output], 0, 0, 0); ImGui::SameLine();
							ImGui::InputScalar("##NumPix", ImGuiDataType_U16, &devices[i]->OutputPixels[output], 0, 0, 0); ImGui::SameLine();
							ImGui::InputScalar("##NullPix", ImGuiDataType_U8, &devices[i]->OutputNull[output], 0, 0, 0); ImGui::SameLine();
							ImGui::InputScalar("##ZigZag", ImGuiDataType_U16, &devices[i]->OutputZig[output], 0, 0, 0); ImGui::SameLine();
							ImGui::InputScalar("##Group", ImGuiDataType_U16, &devices[i]->OutputGrouping[output], 0, 0, 0); ImGui::SameLine();
							ImGui::InputScalar("%##BrightLim", ImGuiDataType_U8, &devices[i]->OutputBrightness[output], 0, 0, 0); ImGui::SameLine();
							tempReversed[output] = (bool)devices[i]->OutputReverse[output];
							if (ImGui::Checkbox("##Reversed", &tempReversed[output])) {
								devices[i]->OutputReverse[output] = (uint8_t)tempReversed[output];
							}
							ImGui::PopID();
						}

						ImGui::PopItemWidth();

						//ImGui::Text("DMX512 Outputs");
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("LEDs"))
					{
						ImGui::PushItemWidth(120);
						ImGui::Combo("Pixel IC", &devices[i]->CurrentDriver, devices[i]->DriverNames, devices[i]->NumDrivers);
						ImGui::Combo("Clock Speed", &devices[i]->CurrentDriverSpeed, DriverSpeedsMhz, 12);
						bool tempExpanded = (bool)devices[i]->CurrentDriverExpanded;
						if (ImGui::Checkbox("Expanded Mode", &tempExpanded)) {
							devices[i]->CurrentDriverExpanded = (uint8_t)tempExpanded;
						}


						int * tempOutputColOrder = new int[devices[i]->NumOutputs];

						for (uint8_t output = 0; output < devices[i]->NumOutputs; output++) {
							ImGui::PushID(output);
							ImGui::Text("Output %i", output + 1); ImGui::SameLine();
							tempOutputColOrder[output] = devices[i]->OutputColOrder[output];
							if (ImGui::Combo("Order", &tempOutputColOrder[output], RGBW_Order, 24)) {
								devices[i]->OutputColOrder[output] = (uint8_t)tempOutputColOrder[output];
							}
							ImGui::PopID();
						}

						ImGui::Separator();
						ImGui::Text("Gamma Correction only applied to chips that are higher then 8bit:");

						if (ImGui::SliderFloat("Red", &devices[i]->Gammaf[0], 1.0, 3.0, "%.01f")) {
							devices[i]->Gamma[0] = (int)(devices[i]->Gammaf[0] * 10);
						};
						if (ImGui::SliderFloat("Green", &devices[i]->Gammaf[1], 1.0, 3.0, "%.01f")) {
							devices[i]->Gamma[1] = (int)(devices[i]->Gammaf[1] * 10);
						};
						if (ImGui::SliderFloat("Blue", &devices[i]->Gammaf[2], 1.0, 3.0, "%.01f")) {
							devices[i]->Gamma[2] = (int)(devices[i]->Gammaf[2] * 10);
						};
						if (ImGui::SliderFloat("White", &devices[i]->Gammaf[3], 1.0, 3.0, "%.01f")) {
							devices[i]->Gamma[3] = (int)(devices[i]->Gammaf[3] * 10);
						};
						ImGui::PopItemWidth();
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Test"))
					{
						ImGui::PushItemWidth(200);

						if (ImGui::Combo("Set Test", &devices[i]->TestMode, TestModes, 9)) {
							setTest(i);
						}

						if ((devices[i]->TestMode == 6) || (devices[i]->TestMode == 8)) {
							float tempTestCols[4];
							tempTestCols[0] = ((float)devices[i]->TestCols[0] / 255);
							tempTestCols[1] = ((float)devices[i]->TestCols[1] / 255);
							tempTestCols[2] = ((float)devices[i]->TestCols[2] / 255);
							tempTestCols[3] = ((float)devices[i]->TestCols[3] / 255);

							if (ImGui::ColorEdit4("Test Colour", tempTestCols)) {
								devices[i]->TestCols[0] = (int)(tempTestCols[0] * 255);
								devices[i]->TestCols[1] = (int)(tempTestCols[1] * 255);
								devices[i]->TestCols[2] = (int)(tempTestCols[2] * 255);
								devices[i]->TestCols[3] = (int)(tempTestCols[3] * 255);
							}
							setTest(i);
						}

						ImGui::PopItemWidth();

						ImGui::EndTabItem();
					}


					if (ImGui::BeginTabItem("Misc"))
					{
						ImGui::PushItemWidth(200);
						char sName[40];
						memcpy(sName, devices[i]->Nickname, 40);
						if (ImGui::InputText("Nickname", sName, 40)) {
							memset(devices[i]->Nickname, 0, 40);
							strcpy(devices[i]->Nickname, sName);
						}
						ImGui::PopItemWidth();

						ImGui::PushItemWidth(30);

						ImGui::InputScalar("Fan Control On Temp", ImGuiDataType_U8, &devices[i]->MaxTargetTemp, 0, 0, 0);
						ImGui::PopItemWidth();

						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();

					ImGui::Separator();

					if (ImGui::Button("Update"))
					{
						updateDevice(i);
					}

					ImGui::TreePop();
				}
			}

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

		if (b_refreshAdaptorsRequest) {
			refreshAdaptors();
			b_refreshAdaptorsRequest = false;
		}
		if (b_pollRequest) {
			poll();
			b_pollRequest = false;
		}
		if (id_networkConfigRequest > -1) {
			bc_networkConfig(id_networkConfigRequest);
			id_networkConfigRequest = -1;
		}
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
