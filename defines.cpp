#include "defines.h"

const char* SyncTypes[3] = {
		"Match Static IP",
		"Match Nickname",
		"Match MAC addres"
};

const char* SortTypes[4] = {
		"Sort ...",
		"Current IP",
		"Static IP",
		"Nickname"
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

std::string macString(uint8_t * address) {
	std::stringstream ss;

	for (int i(0); i < 6; i++) {
		ss << std::hex << std::uppercase << std::setw(2) << static_cast<int>(address[i]);
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

std::vector<std::string> splitter(std::string in_pattern, std::string& content) {
	std::vector<std::string> split_content;

	std::regex pattern(in_pattern);
	std::copy(std::sregex_token_iterator(content.begin(), content.end(), pattern, -1),
		std::sregex_token_iterator(), std::back_inserter(split_content));
	return split_content;
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

void setEndUniverseChannel(uint16_t startUniverse, uint16_t startChannel, uint16_t pixelCount, uint16_t outputGrouping, uint16_t &endUniverse, uint16_t &endChannel) {
	pixelCount *= outputGrouping;
	uint16_t pixelChannels = (3 * pixelCount); // R-G-B data
	uint16_t pixelUniverses = ((float)(startChannel + pixelChannels) / 510.f);

	endUniverse = startUniverse + pixelUniverses;
	endChannel = (startChannel + pixelChannels - 1) % 510;
}

void load_ipStr(std::string ipStr, uint8_t *address)
{
	int ip1, ip2, ip3, ip4;
	sscanf(ipStr.c_str(), "%i.%i.%i.%i", &ip1, &ip2, &ip3, &ip4);
	address[0] = ip1;
	address[1] = ip2;
	address[2] = ip3;
	address[3] = ip4;
}

void load_macStr(std::string macStr, uint8_t *address)
{
	int mac1, mac2, mac3, mac4, mac5, mac6;
	sscanf(macStr.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x", &mac1, &mac2, &mac3, &mac4, &mac5, &mac6);
	address[0] = mac1;
	address[1] = mac2;
	address[2] = mac3;
	address[3] = mac4;
	address[4] = mac5;
	address[5] = mac6;
}