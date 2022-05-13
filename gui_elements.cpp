#include "gui_elements.h"
#include "standard_json_config.h"

pt::ptree pt_json_device;

uint32_t COL_GREY = IM_COL32(80, 80, 80, 255);
uint32_t COL_LIGHTGREY = IM_COL32(180, 180, 180, 255);
uint32_t COL_GREEN = IM_COL32(0, 180, 0, 255);
uint32_t COL_RED = IM_COL32(180, 0, 0, 255);

std::vector<sAdvatekDevice*> foundDevices;
std::vector<std::pair<sAdvatekDevice*, sAdvatekDevice*>> syncDevices;
sAdvatekDevice* syncDevice;

advatek_manager adv;
sImportOptions userImportOptions = sImportOptions();
sImportOptions virtualImportOptions = sImportOptions();

std::string adaptor_string = "None";
std::string json_device_string = "Select Device";
std::string vDeviceString = "New ...";
std::string result = "";
std::string vDeviceData = "";

double currTime = 0;
double lastPoll = 0;
double lastSoftPoll = 0;
double lastTime = 0;

float rePollTime = 3;
float testCycleSpeed = 0.5;
float scale = 1;

int b_testPixelsReady = true;
int b_pollRequest = 0;
int b_refreshAdaptorsRequest = 0;
int b_newVirtualDeviceRequest = 0;
int b_pasteToNewVirtualDevice = 0;
int b_clearVirtualDevicesRequest = 0;
int b_copyAllConnectedToVirtualRequest = 0;
int iClearVirtualDeviceID = -1;
int syncConnectedDeviceRequest = -1;
int b_vDevicePath = false;
int current_json_device = -1;
int current_sync_type = 0;
int b_syncAllRequest = 0;

bool logOpen = true;

bool SliderInt8(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
	return ImGui::SliderScalar(label, ImGuiDataType_U8, v, &v_min, &v_max, format, flags);
}

bool SliderInt16(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
	return ImGui::SliderScalar(label, ImGuiDataType_U16, v, &v_min, &v_max, format, flags);
}

AppLog applog;

void setupWindow(GLFWwindow*& window, int& window_w, int& window_h, float& scale)
{
	if (!glfwInit()) exit(0);

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

	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

	float xscale = mode->width / 1920.f;
	float yscale = mode->height / 1080.f;
	window_w = 800 * xscale;
	window_h = 600 * yscale;
	scale = std::max(xscale, yscale);

	int center_x = (mode->width / 2) - (window_w / 2);
	int center_y = (mode->height / 2) - (window_h / 2);

	// Create window with graphics context
	window = glfwCreateWindow(window_w, window_h, "Advatek Assistor", NULL, NULL);

	glfwSetWindowPos(window, center_x, center_y);

	if (window == NULL) exit(0);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync


						 // Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	scaleToScreenDPI(scale, io);

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
}

void scaleToScreenDPI(float& scale, ImGuiIO& io)
{
	ImGui::GetStyle().ScaleAllSizes(scale);
	ImFontConfig fc = ImFontConfig();
	fc.OversampleH = fc.OversampleV = scale;
	fc.SizePixels = 13.f * scale;
	io.Fonts->AddFontDefault(&fc);
}

void showResult(std::string& result, float scale) {
	if (result.empty() == false)
		ImGui::OpenPopup("Result");

	//Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f * scale, 0.5f * scale));
	//ImGui::SetNextWindowSize(ImVec2(400, 300));
	ImGui::SetNextWindowSizeConstraints(ImVec2(300.f * scale, -1.f * scale), ImVec2(INFINITY, -1.f * scale));

	if (ImGui::BeginPopupModal("Result", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		// First line is header
		std::vector<std::string> lines = splitter("\n", result);

		ImGui::Text(lines[0].c_str());
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		for (std::vector<std::string>::size_type i = 1; i != lines.size(); i++) {
			ImGui::TextWrapped(lines[i].c_str());
		}

		ImGui::PopStyleVar();
		ImGui::Spacing();

		ImGui::PushStyleColor(ImGuiCol_Text, COL_GREY);
		ImGui::TextWrapped("Please click the Update Network or Update Settings to save these changes to the controller.");
		ImGui::PopStyleColor();

		ImGui::Spacing();
		if (ImGui::Button("OK", ImVec2(120 * scale, 0))) {
			result = std::string("");
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void button_update_controller_settings(int i) {
	if (ImGui::Button("Update Settings"))
	{
		adv.updateDevice(i);
		b_pollRequest = true;
	}
}

void colouredText(const char* txt, uint32_t color) {
	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::TextWrapped(txt);
	ImGui::PopStyleColor();
}

void importUI(sAdvatekDevice* device, sImportOptions& importOptions, float scale) {
	if (ImGui::BeginPopupModal("Import", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		// Read in devices
		pt::ptree pt_json_devices;
		std::vector<pt::ptree> loadedJsonDevices;
		std::vector<std::string> jsonDeviceNames;

		std::stringstream ss;
		ss << importOptions.json;
		pt::read_json(ss, pt_json_devices);

		if (pt_json_devices.count("advatek_devices") > 0) {
			// advatek_device = advatek_devices.get_child("advatek_devices").front().second;
			for (auto& json_device : pt_json_devices.get_child("advatek_devices")) {
				//	advatek_device = device.second;
				loadedJsonDevices.emplace_back(json_device.second);
				std::string sModelName = json_device.second.get<std::string>("Model");
				std::string sNickName = json_device.second.get<std::string>("Nickname");
				jsonDeviceNames.emplace_back(std::string(sModelName + " " + sNickName));
			}
		}
		else {
			pt_json_device = pt_json_devices;
		}

		ImGui::Text("What needs importing?");
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		if (loadedJsonDevices.size() > 0) {
			if (ImGui::BeginCombo("###jsondevices", json_device_string.c_str(), 0))
			{
				for (int n = 0; n < jsonDeviceNames.size(); n++)
				{
					const bool is_selected = (current_json_device == n);
					if (ImGui::Selectable(jsonDeviceNames[n].c_str(), is_selected))
					{
						json_device_string = jsonDeviceNames[n];
						pt_json_device = loadedJsonDevices[n];
					}
				}
				ImGui::EndCombo();
			}
			ImGui::Spacing();
		}

		ImGui::Checkbox("Network", &importOptions.network);
		ImGui::Checkbox("Ethernet Control (Mapping)", &importOptions.ethernet_control);
		ImGui::Checkbox("DMX512 Outputs", &importOptions.dmx_outputs);
		ImGui::Checkbox("LED Settings", &importOptions.led_settings);
		ImGui::Checkbox("Nickname", &importOptions.nickname);
		ImGui::Checkbox("Fan On Temp", &importOptions.fan_on_temp);

		ImGui::PopStyleVar();
		ImGui::Spacing();

		if (ImGui::Button("Import", ImVec2(120 * scale, 0))) {
			std::stringstream jsonStringStream;
			write_json(jsonStringStream, pt_json_device);
			importOptions.json = jsonStringStream.str();
			importOptions.userSet = true;
			ImGui::CloseCurrentPopup();
			current_json_device = -1;
			pt_json_device.clear();
		}

		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120 * scale, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}
}

void button_import_export_JSON(sAdvatekDevice* device) {

	if (ImGui::Button("Copy"))
	{
		adv.copyToMemoryDevice(device);
		applog.AddLog("[INFO] Copied data from %s (%s)\n", device->Nickname, ipString(device->CurrentIP).c_str());
	}

	ImGui::SameLine();

	if (adv.memoryDevices.size() == 1) {
		if (ImGui::Button("Paste"))
		{
			adv.getJSON(adv.memoryDevices[0], userImportOptions);
			ImGui::OpenPopup("Import");
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Import JSON")) {
		auto path = pfd::open_file("Select a file", ".", { "JSON Files", "*.json *.JSON" }).result();
		if (!path.empty()) {
			applog.AddLog("[INFO] Loading JSON file from %s\n", path.at(0).c_str());
			boost::property_tree::ptree advatek_devices;
			boost::property_tree::read_json(path.at(0), advatek_devices);

			std::string result = adv.validateJSON(advatek_devices);
			if (!result.empty()) {
				applog.AddLog("[ERROR] Not a valid JSON file.\n");
			}
			else {
				std::stringstream jsonStringStream;
				write_json(jsonStringStream, advatek_devices);

				userImportOptions.json = jsonStringStream.str();

				ImGui::OpenPopup("Import");
			}
		}
	}

	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f * scale, 0.5f * scale));

	importUI(device, userImportOptions, scale);

	if (userImportOptions.userSet) {
		result = adv.importJSON(device, userImportOptions);
		userImportOptions = sImportOptions();
	}

	ImGui::SameLine();

	if (ImGui::Button("Export JSON"))
	{
		std::stringstream ss;
		ss << device->Nickname << ".json";
		std::string filename(ss.str());
		std::replace(filename.begin(), filename.end(), ' ', '_');

		auto path = pfd::save_file("Select a file", filename).result();
		if (!path.empty()) {
			adv.exportJSON(device, path);
		}

	}
}

void showDevices(std::vector<sAdvatekDevice*>& devices, bool isConnected, float scale) {

	ImGui::Spacing();

	for (uint8_t i = 0; i < devices.size(); i++) {
		bool deviceInRange = adv.ipInRange(adaptor_string, devices[i]);
		std::stringstream Title;
		Title << " " << devices[i]->Model << "	" << devices[i]->Firmware << "	";
		if (isConnected) {
			Title << ipString(devices[i]->CurrentIP);
		}
		else {
			Title << ipString(devices[i]->StaticIP);
		}
		Title << "	" << "Temp: " << (float)devices[i]->Temperature * 0.1 << "	" << devices[i]->Nickname;
		Title << "###" << devices[i]->uid;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f * scale, 8.f * scale));
		bool node_open = ImGui::TreeNodeEx(Title.str().c_str(), ImGuiSelectableFlags_SpanAllColumns);
		ImGui::PopStyleVar();

		if (node_open)
		{
			//ImGui::Columns(1);
			ImGui::Spacing();

			if (isConnected) {
				if (devices[i]->ProtVer >= 8) {
					if (ImGui::Button("ID"))
					{
						adv.identifyDevice(i, 20);
					}
					ImGui::SameLine();
				}
			}
			else {
				if (ImGui::Button("Delete"))
				{
					iClearVirtualDeviceID = i;
				}
				ImGui::SameLine();
			}

			button_import_export_JSON(devices[i]);

			ImGui::SameLine();

			if (isConnected) {
				if (ImGui::Button("New Virtual Device"))
				{
					adv.copyToNewVirtualDevice(adv.connectedDevices[i]);
					applog.AddLog("[INFO] Copied controller %s %s to new virtual device.\n", adv.connectedDevices[i]->Nickname, ipString(adv.connectedDevices[i]->CurrentIP).c_str());
				}
			}
			else {
				ImGui::PushItemWidth(240 * scale);
				if (ImGui::BeginCombo("###senddevice", "Copy to Connected Device", 0))
				{
					for (int n = 0; n < adv.connectedDevices.size(); n++)
					{
						if (ImGui::Selectable(ipString(adv.connectedDevices[n]->StaticIP).append(" ").append(adv.connectedDevices[n]->Nickname).c_str()))
						{
							adv.copyDevice(adv.virtualDevices[i], adv.connectedDevices[n], false);
							applog.AddLog("[INFO] Copied virtual controller to new device  %s %s.\n", adv.connectedDevices[i]->Nickname, ipString(adv.connectedDevices[i]->CurrentIP).c_str());
						}
					}
					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();
			}

			ImGui::Separator();
			ImGui::Spacing();

			ImGui::BeginTabBar("MyTabBar", ImGuiTabBarFlags_None);
			if (ImGui::BeginTabItem("Network"))
			{
				if (isConnected && !deviceInRange) {
					ImGui::Spacing();
					auto txtCol = IM_COL32(120, 120, 120, 255);
					ImGui::PushStyleColor(ImGuiCol_Text, txtCol);
					ImGui::Text("The IP address settings are not compatible with your adaptor settings.\nChange the IP settings on the device to access all settings.");
					ImGui::PopStyleColor();
					ImGui::Spacing();
				}

				ImGui::Text("Static IP Address:");

				ImGui::PushItemWidth(30 * scale);

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

				ImGui::EndTabItem();
			}

			if (!isConnected || isConnected && deviceInRange) {
				if (ImGui::BeginTabItem("Ethernet Control"))
				{
					int tempProtocol = (int)devices[i]->Protocol;
					if (ImGui::RadioButton("ArtNet", &tempProtocol, 1)) {
						devices[i]->Protocol = 1;
					} ImGui::SameLine();
					if (ImGui::RadioButton("sACN (E1.31)", &tempProtocol, 0)) {
						devices[i]->Protocol = 0;
					}
					ImGui::SameLine();
					bool tempHoldLastFrame = (bool)devices[i]->HoldLastFrame;
					if (ImGui::Checkbox("Hold LastFrame", &tempHoldLastFrame)) {
						devices[i]->HoldLastFrame = tempHoldLastFrame;
					}

					bool tempSimpleConfig = (bool)devices[i]->SimpleConfig;
					if (ImGui::Checkbox("Simple Config", &tempSimpleConfig)) {
						devices[i]->SimpleConfig = tempSimpleConfig;
					}

					ImGui::PushItemWidth(50 * scale);

					if ((bool)devices[i]->SimpleConfig) {
						ImGui::InputScalar("Start Universe", ImGuiDataType_U16, &devices[i]->OutputUniv[0], 0, 0, 0);
						ImGui::InputScalar("Start Channel ", ImGuiDataType_U16, &devices[i]->OutputChan[0], 0, 0, 0);
						ImGui::InputScalar("Pixels Per Output", ImGuiDataType_U16, &devices[i]->OutputPixels[0], 0, 0, 0);
					}
					else {
						bool autoChannels = false;
						ImGui::SameLine();
						ImGui::Checkbox("Automatic Sequence Channels", &autoChannels);

						if (autoChannels) {
							adv.auto_sequence_channels(i);
						}

						bool* tempReversed = new bool[devices[i]->NumOutputs];
						uint16_t* tempEndUniverse = new uint16_t[devices[i]->NumOutputs];
						uint16_t* tempEndChannel = new uint16_t[devices[i]->NumOutputs];

						if (ImGui::BeginTable("advancedTable", 11))
						{
							ImGui::TableSetupColumn(" ");
							ImGui::TableSetupColumn("Start\nUniverse");
							ImGui::TableSetupColumn("Start\nChannel");
							ImGui::TableSetupColumn("End\nUniverse");
							ImGui::TableSetupColumn("End\nChannel");
							ImGui::TableSetupColumn("Num\nPixels");
							ImGui::TableSetupColumn("Null\nPixels");
							ImGui::TableSetupColumn("Zig\nZag");
							ImGui::TableSetupColumn("Group");
							ImGui::TableSetupColumn("Intensity\nLimit");
							ImGui::TableSetupColumn("Reversed");
							ImGui::TableHeadersRow();

							for (int output = 0; output < devices[i]->NumOutputs * 0.5; output++)
							{
								ImGui::TableNextRow();

								setEndUniverseChannel(devices[i]->OutputUniv[output], devices[i]->OutputChan[output], devices[i]->OutputPixels[output], devices[i]->OutputGrouping[output], tempEndUniverse[output], tempEndChannel[output]);
								ImGui::PushID(output);

								ImGui::TableNextColumn();
								ImGui::Text("Output %i", output + 1); ImGui::TableNextColumn();
								ImGui::InputScalar("##StartUniv", ImGuiDataType_U16, &devices[i]->OutputUniv[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##StartChan", ImGuiDataType_U16, &devices[i]->OutputChan[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##EndUniv", ImGuiDataType_U16, &tempEndUniverse[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##EndChan", ImGuiDataType_U16, &tempEndChannel[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##NumPix", ImGuiDataType_U16, &devices[i]->OutputPixels[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##NullPix", ImGuiDataType_U8, &devices[i]->OutputNull[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##ZigZag", ImGuiDataType_U16, &devices[i]->OutputZig[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##Group", ImGuiDataType_U16, &devices[i]->OutputGrouping[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("%##BrightLim", ImGuiDataType_U8, &devices[i]->OutputBrightness[output], 0, 0, 0); ImGui::TableNextColumn();
								tempReversed[output] = (bool)devices[i]->OutputReverse[output];
								if (ImGui::Checkbox("##Reversed", &tempReversed[output])) {
									devices[i]->OutputReverse[output] = (uint8_t)tempReversed[output];
								}
								ImGui::PopID();

							}
							ImGui::EndTable();
						}
					} // End Else/If Simple Config

					ImGui::PopItemWidth();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("DMX512 Outputs"))
				{
					bool* tempDMXOffOn = new bool[devices[i]->NumDMXOutputs];

					ImGui::PushItemWidth(50 * scale);

					for (int DMXoutput = 0; DMXoutput < devices[i]->NumDMXOutputs; DMXoutput++) {
						ImGui::PushID(DMXoutput);
						tempDMXOffOn[DMXoutput] = (bool)devices[i]->DmxOutOn[DMXoutput];

						ImGui::Text("Output %i", DMXoutput + 1);
						ImGui::SameLine();

						if (ImGui::Checkbox("Enabled", &tempDMXOffOn[DMXoutput])) {
							devices[i]->DmxOutOn[DMXoutput] = (uint8_t)tempDMXOffOn[DMXoutput];
						}

						ImGui::SameLine();
						ImGui::InputScalar("Universe", ImGuiDataType_U16, &devices[i]->DmxOutUniv[DMXoutput], 0, 0, 0);
						ImGui::PopID();
					}

					ImGui::PopItemWidth();
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("LEDs"))
				{
					ImGui::PushItemWidth(120 * scale);
					ImGui::Combo("Pixel IC", &devices[i]->CurrentDriver, devices[i]->DriverNames, devices[i]->NumDrivers);
					ImGui::Combo("Clock Speed", &devices[i]->CurrentDriverSpeed, DriverSpeedsMhz, 12);
					bool tempExpanded = (bool)devices[i]->CurrentDriverExpanded;
					if (ImGui::Checkbox("Expanded Mode", &tempExpanded)) {
						devices[i]->CurrentDriverExpanded = (uint8_t)tempExpanded;
					}

					int tempAllColOrder = devices[i]->OutputColOrder[0];
					if (ImGui::Combo("RGB Order ##all", &tempAllColOrder, RGBW_Order, 24)) {
						for (uint8_t output = 0; output < devices[i]->NumOutputs * 0.5; output++) {
							devices[i]->OutputColOrder[output] = (uint8_t)tempAllColOrder;
						}
					}

					//int * tempOutputColOrder = new int[devices[i]->NumOutputs];
					//for (uint8_t output = 0; output < devices[i]->NumOutputs*0.5; output++) {
					//	ImGui::PushID(output);
					//	ImGui::Text("Output %02i", output + 1); ImGui::SameLine();
					//	tempOutputColOrder[output] = devices[i]->OutputColOrder[output];
					//	if (ImGui::Combo("Order", &tempOutputColOrder[output], adv.RGBW_Order, 24)) {
					//		devices[i]->OutputColOrder[output] = (uint8_t)tempOutputColOrder[output];
					//	}
					//	ImGui::PopID();
					//}

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
				if (isConnected) {
					if (ImGui::BeginTabItem("Test"))
					{
						ImGui::Spacing();
						auto txtCol = IM_COL32(120, 120, 120, 255);
						ImGui::PushStyleColor(ImGuiCol_Text, txtCol);
						ImGui::Text("Prior to running test mode all other setings should be saved to controller. (Press 'Update Settings')");
						ImGui::PopStyleColor();
						ImGui::Spacing();

						bool b_setTest = false;

						ImGui::PushItemWidth(200 * scale);

						if (ImGui::Combo("Set Test", &devices[i]->TestMode, TestModes, 9)) {
							devices[i]->TestPixelNum = 0;
							b_testPixelsReady = true;
							b_setTest = true;
						}

						if (devices[i]->ProtVer < 8 && devices[i]->TestMode == 8) {
							ImGui::Text("Device does not support single pixel output...");
							devices[i]->TestMode = 6;
						}

						if ((devices[i]->TestMode == 6) || (devices[i]->TestMode == 8)) {
							if (ImGui::ColorEdit4("Test Colour", devices[i]->tempTestCols)) {
								devices[i]->TestCols[0] = (int)(devices[i]->tempTestCols[0] * 255);
								devices[i]->TestCols[1] = (int)(devices[i]->tempTestCols[1] * 255);
								devices[i]->TestCols[2] = (int)(devices[i]->tempTestCols[2] * 255);
								devices[i]->TestCols[3] = (int)(devices[i]->tempTestCols[3] * 255);
								b_setTest = true;
							}

							if (devices[i]->ProtVer > 7) {

								if (devices[i]->testModeCycleOuputs || devices[i]->testModeCyclePixels) {

									if (currTime - lastTime > testCycleSpeed) {
										if (devices[i]->TestMode == 8) {
											// Set Pixel
											devices[i]->TestPixelNum = (devices[i]->TestPixelNum) % ((int)(devices[i]->OutputPixels[(int)devices[i]->TestOutputNum - 1])) + 1;

											if (devices[i]->TestPixelNum == 1) {
												b_testPixelsReady = true;
											}
											else {
												b_testPixelsReady = false;
											}
										}
										if (devices[i]->testModeCycleOuputs && b_testPixelsReady) {
											devices[i]->TestOutputNum = (devices[i]->TestOutputNum) % ((int)(devices[i]->NumOutputs * 0.5)) + 1;
										}
										lastTime = currTime;
										b_setTest = true;
									}
									/*
									for (uint8_t output = 0; output < devices[i]->NumOutputs*0.5; output++) {
										ImGui::PushID(output);
										ImGui::Text("Output %02i", output + 1);

										ImGui::PopID();
									}*/
								}

								bool testModeCycleOuputs = devices[i]->testModeCycleOuputs;
								if (ImGui::Checkbox("Cycle Outputs", &testModeCycleOuputs)) {
									devices[i]->testModeCycleOuputs = (bool)testModeCycleOuputs;
								}

								if (devices[i]->TestMode == 8) {
									bool testModeCyclePixels = devices[i]->testModeCyclePixels;
									if (ImGui::Checkbox("Cycle Pixels", &testModeCyclePixels)) {
										devices[i]->testModeCyclePixels = (bool)testModeCyclePixels;
									}
								}

								ImGui::SliderFloat("Speed", &testCycleSpeed, 5.0, 0.01, "%.01f");

							}

						} // End ProtVer 8+

						if (devices[i]->ProtVer > 7) {

							if (SliderInt8("Output (All 0)", (int*)&devices[i]->TestOutputNum, 0, (devices[i]->NumOutputs * 0.5))) {
								b_setTest = true;
							}

							if (devices[i]->TestMode == 8) {
								if (SliderInt16("Pixels (All 0)", (int*)&devices[i]->TestPixelNum, 0, (devices[i]->OutputPixels[(int)devices[i]->TestOutputNum - 1]))) {
									b_setTest = true;
								}
							}
						}

						if (b_setTest) adv.setTest(i);

						ImGui::PopItemWidth();
						ImGui::EndTabItem();
					} // End Test Tab
				}

				if (ImGui::BeginTabItem("Misc"))
				{
					if (isConnected) {
						ImGui::Text("MAC: %s", macString(devices[i]->Mac).c_str());
					}
					else {
						ImGui::Text("MAC: ");
						ImGui::PushItemWidth(25 * scale);
						ImGui::SameLine(); ImGui::InputScalar("###Mac01", ImGuiDataType_U8, &devices[i]->Mac[0], 0, 0, "%02X");
						ImGui::SameLine(); ImGui::InputScalar("###Mac02", ImGuiDataType_U8, &devices[i]->Mac[1], 0, 0, "%02X");
						ImGui::SameLine(); ImGui::InputScalar("###Mac03", ImGuiDataType_U8, &devices[i]->Mac[2], 0, 0, "%02X");
						ImGui::SameLine(); ImGui::InputScalar("###Mac04", ImGuiDataType_U8, &devices[i]->Mac[3], 0, 0, "%02X");
						ImGui::SameLine(); ImGui::InputScalar("###Mac05", ImGuiDataType_U8, &devices[i]->Mac[4], 0, 0, "%02X");
						ImGui::SameLine(); ImGui::InputScalar("###Mac06", ImGuiDataType_U8, &devices[i]->Mac[5], 0, 0, "%02X");
						ImGui::PopItemWidth();
					}

					ImGui::Text("Protocol Version: %s", std::to_string(devices[i]->ProtVer).c_str());

					ImGui::PushItemWidth(200 * scale);
					char sName[40];
					memcpy(sName, devices[i]->Nickname, 40);
					if (ImGui::InputText("Nickname", sName, 40)) {
						memset(devices[i]->Nickname, 0, 40);
						strcpy(devices[i]->Nickname, sName);
					}
					ImGui::PopItemWidth();

					ImGui::PushItemWidth(30 * scale);

					ImGui::InputScalar("Fan Control On Temp", ImGuiDataType_U8, &devices[i]->MaxTargetTemp, 0, 0, 0);
					ImGui::PopItemWidth();

					if (isConnected) {
						int* tempVoltage = new int[devices[i]->NumBanks];

						for (uint8_t bank = 0; bank < devices[i]->NumBanks; bank++) {
							ImGui::PushID(bank);
							ImGui::Text("Bank %i: %.2f V", bank + 1, ((float)devices[i]->VoltageBanks[bank] / 10.f));
							ImGui::PopID();
						}
					}

					ImGui::EndTabItem();

				}
			}

			ImGui::EndTabBar();

			ImGui::Spacing();
			ImGui::Spacing();

			if (isConnected && !deviceInRange) {
				if (ImGui::Button("Update Network"))
				{
					adv.bc_networkConfig(i);
					b_pollRequest = true;
				}
			}
			else if (isConnected) {
				button_update_controller_settings(i);
			}

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::TreePop();
		}
	}
}

void showSyncDevice(const uint8_t& i, bool& canSyncAll, bool& inSyncAll)
{
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	colouredText("(Virtual Device)", COL_GREY);  ImGui::SameLine();
	ImGui::Text(ipString(adv.virtualDevices[i]->StaticIP).c_str()); ImGui::SameLine();
	colouredText(adv.virtualDevices[i]->Nickname, COL_LIGHTGREY);

	bool deviceInRange = adv.ipInRange(adaptor_string, adv.virtualDevices[i]);
	if (!deviceInRange) {
		colouredText("Has IP address settings not compatible with your adaptor settings.", COL_RED);
		colouredText("Change the IP settings of your adaptor.", COL_RED);
		canSyncAll = false;
		return;
	}

	switch (current_sync_type) {
	case 0: // Match Static IP
		foundDevices = adv.getDevicesWithStaticIP(adv.connectedDevices, ipString(adv.virtualDevices[i]->StaticIP));
		if (foundDevices.size() != 1) {
			if (foundDevices.size()) {
				colouredText(std::string("Multiple connected devices with static IP ").append(ipString(adv.virtualDevices[i]->StaticIP)).c_str(), COL_RED);
			}
			else {
				colouredText(std::string("No connected device with static IP ").append(ipString(adv.virtualDevices[i]->StaticIP)).c_str(), COL_RED);
			}
			canSyncAll = false;
			return;
		}
		break;
	case 1: // Match Nickname
		foundDevices = adv.getDevicesWithNickname(adv.connectedDevices, std::string(adv.virtualDevices[i]->Nickname));
		if (foundDevices.size() != 1) {
			if (foundDevices.size()) {
				colouredText(std::string("Multiple connected devices with Nickname ").append(adv.virtualDevices[i]->Nickname).c_str(), COL_RED);
			}
			else {
				colouredText(std::string("No connected device with Nickname ").append(adv.virtualDevices[i]->Nickname).c_str(), COL_RED);
			}
			canSyncAll = false;
			return;
		}
		break;
	case 2: // Match MAC
		foundDevices = adv.getDevicesWithMac(adv.connectedDevices, macString(adv.virtualDevices[i]->Mac));
		if (foundDevices.size() != 1) {
			if (foundDevices.size()) {
				// This should never happen :)
				colouredText(std::string("Whoah!! Multiple connected devices with MAC address ").append(macString(adv.virtualDevices[i]->Mac)).c_str(), COL_RED);
			}
			else {
				colouredText(std::string("No connected device with MAC address ").append(macString(adv.virtualDevices[i]->Mac)).c_str(), COL_RED);
			}
			canSyncAll = false;
			return;
		}
		break;
	default:
		return;
		break;
	}

	if (adv.deviceCompatible(adv.virtualDevices[i], foundDevices[0])) {
		if (adv.devicesInSync(adv.virtualDevices[i], foundDevices[0])) {
			colouredText("Device in sync ...", COL_GREEN);
		}
		else {
			inSyncAll = false;
			syncDevices.emplace_back(adv.virtualDevices[i], foundDevices[0]);
			if (ImGui::Button("Update Connected Device"))
			{
				syncDevice = foundDevices[0];
				syncConnectedDeviceRequest = i;
			}
		}
		return;
	}
	else {
		colouredText("Device not compatable ...", COL_RED);
		canSyncAll = false;
		return;
	}
}

void showWindow(GLFWwindow*& window, int window_w, int window_h, float scale)
{
	{
		// Create a window and append into it.
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(window_w, window_h));

		ImGuiWindowFlags window_flags = 0;
		window_flags |= ImGuiWindowFlags_NoTitleBar;
		window_flags |= ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoResize;
		window_flags |= ImGuiWindowFlags_NoCollapse;

		ImGui::Begin("Advatek Assistor", NULL, window_flags);

		showResult(result, scale);

		if (ImGui::Button("Refresh Adaptors"))
		{
			b_refreshAdaptorsRequest = true;
		} ImGui::SameLine();

		ImGui::PushItemWidth(130 * scale);

		if (ImGui::BeginCombo("###Adaptor", adaptor_string.c_str(), 0))
		{
			for (int n = 0; n < adv.networkAdaptors.size(); n++)
			{
				const bool is_selected = (adv.currentAdaptor == n);
				if (ImGui::Selectable(adv.networkAdaptors[n].c_str(), is_selected))
				{
					adaptor_string = adv.networkAdaptors[n];

					adv.setCurrentAdaptor(n);

					b_pollRequest = true;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Spacing();
		// Connected Devices && Virtual Devices
		if (ImGui::BeginTabBar("Devices")) {
			std::stringstream tabTitleConnected;
			std::stringstream tabTitleVirtual;
			tabTitleConnected << "Connected Devices (";
			tabTitleVirtual << "Virtual Devices (";
			tabTitleConnected << adv.connectedDevices.size();
			tabTitleVirtual << adv.virtualDevices.size();
			tabTitleConnected << ")";
			tabTitleVirtual << ")";
			tabTitleConnected << "###connectedDevices";
			tabTitleVirtual << "###virtualDevices";

			if (ImGui::BeginTabItem(tabTitleConnected.str().c_str()))
			{
				ImGui::Spacing();
				if (ImGui::Button("Search"))
				{
					b_pollRequest = true;
				}

				if (adv.connectedDevices.size() >= 1) {
					ImGui::SameLine();
					if (ImGui::Button("Export All")) {
						auto path = pfd::save_file("Select a file", "controllerPack.json").result();
						if (!path.empty()) {
							adv.exportJSON(adv.connectedDevices, path);
						}
					}
					ImGui::SameLine();
					if (ImGui::Button("Copy All to Virtual Devices")) {
						b_copyAllConnectedToVirtualRequest = true;
					}
				}

				if (adv.connectedDevices.size() > 1) {
					ImGui::SameLine();

					static int item_current = 0;
					if (ImGui::Combo("###SortConnectedDevices", &item_current, SortTypes, IM_ARRAYSIZE(SortTypes))) {
						adv.sortDevices(adv.connectedDevices, item_current);
						item_current = 0;
					}
				}
				ImGui::Spacing();

				showDevices(adv.connectedDevices, true, scale);

				// END Connected Devices
				ImGui::EndTabItem();
			}

			// -------------------------------------- START SYNC
			if (ImGui::BeginTabItem("<-"))
			{
				ImGui::Spacing();
				ImGui::PushItemWidth(182 * scale);
				ImGui::Combo("###SyncTypes", &current_sync_type, SyncTypes, IM_ARRAYSIZE(SyncTypes));
				ImGui::PopItemWidth();
				ImGui::SameLine();

				bool canSyncAll = true;
				bool inSyncAll = true;

				// Check static IP of all virtual devices for conflict
				// In name mode check for dupplicate names

				syncDevices.clear();

				for (uint8_t i = 0; i < adv.virtualDevices.size(); i++) {
					ImGui::PushID(i);
					showSyncDevice(i, canSyncAll, inSyncAll);
					ImGui::PopID();
				}

				ImGui::Spacing();

				if (canSyncAll && !inSyncAll) {
					ImGui::Separator();
					ImGui::Spacing();
					if (ImGui::Button("Update All"))
					{
						b_syncAllRequest = true;
					}
					ImGui::Spacing();
				}

				ImGui::EndTabItem();
			}
			// -------------------------------------- END SYNC

			if (ImGui::BeginTabItem(tabTitleVirtual.str().c_str()))
			{
				ImGui::Spacing();

				if (ImGui::BeginCombo("###NewVirtualDevice", vDeviceString.c_str(), 0))
				{

					for (const auto& jsonData : JSONControllers)
					{
						const bool is_selected = (vDeviceString.c_str() == jsonData[0].c_str());
						if (ImGui::Selectable(jsonData[0].c_str(), is_selected))
						{
							virtualImportOptions = sImportOptions();
							virtualImportOptions.json = jsonData[1];
							virtualImportOptions.init = true;
							b_newVirtualDeviceRequest = true;
						}
					}
					ImGui::EndCombo();
				}

				ImGui::SameLine();
				if (ImGui::Button("Import JSON")) {
					auto path = pfd::open_file("Select a file", ".", { "JSON Files", "*.json *.JSON" }).result();
					if (!path.empty()) {
						applog.AddLog("[INFO] Loading JSON file from %s\n", path.at(0).c_str());
						boost::property_tree::ptree advatek_devices;
						boost::property_tree::read_json(path.at(0), advatek_devices);

						std::string result = adv.validateJSON(advatek_devices);

						if (!result.empty()) {
							applog.AddLog("[ERROR] Not a valid JSON file.\n");
						}
						else {
							std::stringstream jsonStringStream;
							write_json(jsonStringStream, advatek_devices);

							virtualImportOptions = sImportOptions();
							virtualImportOptions.json = jsonStringStream.str();
							virtualImportOptions.init = true;

							b_newVirtualDeviceRequest = true;
						}
					}
				}

				if (adv.virtualDevices.size() > 0) {
					ImGui::SameLine();
					if (ImGui::Button("Export All")) {
						auto path = pfd::save_file("Select a file", "controllerPack.json").result();
						if (!path.empty()) {
							adv.exportJSON(adv.virtualDevices, path);
						}
					}
				}

				if (adv.memoryDevices.size() == 1) {
					ImGui::SameLine();
					if (ImGui::Button("Paste"))
					{
						b_pasteToNewVirtualDevice = true;
					}
				}

				if (adv.virtualDevices.size() >= 1) {
					ImGui::SameLine();
					if (ImGui::Button("Clear"))
					{
						b_clearVirtualDevicesRequest = true;
					}
				}

				if (adv.virtualDevices.size() > 1) {
					ImGui::SameLine();

					static int item_current = 0;
					if (ImGui::Combo("###SortVirtualDevices", &item_current, SortTypes, IM_ARRAYSIZE(SortTypes))) {
						adv.sortDevices(adv.virtualDevices, item_current);
						item_current = 0;
					}
				}

				ImGui::Spacing();

				showDevices(adv.virtualDevices, false, scale);

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Separator();

		applog.Draw("Advatek Assistor", &logOpen, scale);

		ImGui::End();
	}

	// Rendering
	ImGui::Render();
	int display_w, display_h;
	glfwGetFramebufferSize(window, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	glfwSwapBuffers(window);
}

void processUpdateRequests()
{
	if (b_refreshAdaptorsRequest) {
		b_refreshAdaptorsRequest = false;
		adv.refreshAdaptors();
		if (adv.networkAdaptors.size() > 0) {
			adaptor_string = adv.networkAdaptors[0];
			adv.poll();
			applog.AddLog(("[INFO] Polling using network adaptor " + adaptor_string + " ...\n").c_str());
			lastPoll = currTime;
		}
		else {
			adaptor_string = "None";
		}
	}

	if (b_pollRequest) {
		adv.poll();
		b_pollRequest = false;
	}

	if (b_newVirtualDeviceRequest) {
		adv.addVirtualDevice(virtualImportOptions);
		b_newVirtualDeviceRequest = false;
	}

	if (b_pasteToNewVirtualDevice) {
		adv.pasteToNewVirtualDevice();
		applog.AddLog("[INFO] Pasted data into new virtual device\n");
		b_pasteToNewVirtualDevice = false;
	}

	if (b_clearVirtualDevicesRequest) {
		adv.clearDevices(adv.virtualDevices);
		b_clearVirtualDevicesRequest = false;
	}

	if (iClearVirtualDeviceID >= 0) {
		adv.virtualDevices.erase(adv.virtualDevices.begin() + iClearVirtualDeviceID);
		iClearVirtualDeviceID = -1;
	}

	if (b_copyAllConnectedToVirtualRequest) {
		for (auto device : adv.connectedDevices) {
			boost::property_tree::ptree advatek_device;
			adv.getJSON(device, advatek_device);
			sImportOptions conImportOptions = sImportOptions();
			conImportOptions.init = true;
			adv.addVirtualDevice(advatek_device, conImportOptions);
		}
		b_copyAllConnectedToVirtualRequest = false;
	}

	if (syncConnectedDeviceRequest > -1) {
		adv.updateConnectedDevice(adv.virtualDevices[syncConnectedDeviceRequest], syncDevice);
		syncConnectedDeviceRequest = -1;
		adv.removeConnectedDevice(macString(syncDevice->Mac));
	}

	if (b_syncAllRequest) {
		for (uint8_t i = 0; i < syncDevices.size(); i++) {
			adv.updateConnectedDevice(syncDevices[i].first, syncDevices[i].second);
			adv.removeConnectedDevice(macString(syncDevices[i].second->Mac));
		}
		b_syncAllRequest = false;
	}
}
