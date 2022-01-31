#pragma once

#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/exception/diagnostic_information.hpp>

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

typedef struct tAdvatekDevice {
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
	int* TestCycleCols; //[NumOutputs]
	bool TestModeCycle = false;
} sAdvatekDevice;

std::string macString(uint8_t * address);
std::string ipString(uint8_t * address);

//extern const char* RGBW_Order[24];
extern const char* DriverTypes[3];
extern const char* DriverSpeeds[5];
extern const char* DriverSpeedsMhz[12];
extern const char* TestModes[9];

void insertSwapped16(std::vector<uint8_t> &dest, uint16_t* pData, int32_t size);

bool deviceExist(uint8_t * Mac);

extern boost::asio::io_context io_context;
extern boost::asio::ip::udp::endpoint receiver;
extern boost::asio::ip::udp::socket s_socket;
extern boost::asio::ip::udp::socket r_socket;
extern boost::asio::ip::tcp::resolver resolver;
extern boost::asio::ip::tcp::resolver::query query;

void setEndUniverseChannel(int startUniverse, int startChannel, int pixelCount, int outputGrouping, int &endUniverse, int &endChannel);

class advatek_manager
{
public:
	bool deviceExist(uint8_t * Mac);
	std::string macStr(uint8_t * address);
	std::string ipStr(uint8_t * address);

	std::vector <std::string> networkAdaptors;
	int currentAdaptor = 0;
	
	std::vector<sAdvatekDevice*> devices;

	void updateDevice(int d);
	void setTest(int d);
	void clearDevices();
	void bc_networkConfig(int d);
	void poll();
	void process_opPollReply(uint8_t * data);
	void process_opTestAnnounce(uint8_t * data);
	void process_udp_message(uint8_t * data);
	void send_udp_message(std::string ip_address, int port, bool b_broadcast, std::vector<uint8_t> message);
	void unicast_udp_message(std::string ip_address, std::vector<uint8_t> message);
	void broadcast_udp_message(std::vector<uint8_t> message);

	void refreshAdaptors();
	
	constexpr static const char* RGBW_Order[24] = {
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

private:

};
