#include "gui_elements.h"
#include "standard_json_config.h"

uint32_t COL_GREY = IM_COL32(80, 80, 80, 255);
uint32_t COL_LIGHTGREY = IM_COL32(180, 180, 180, 255);
uint32_t COL_GREEN = IM_COL32(0, 180, 0, 255);
uint32_t COL_RED = IM_COL32(180, 0, 0, 255);

float eness_colourcode_ouptput[16][4] = {
	{0,255,0,255},
	{255,255,0,255},
	{255,255,255,255},
	{255,0,255,255},
	{0,255,0,255},
	{255,255,0,255},
	{255,255,255,255},
	{255,0,255,255},
	{0,255,0,255},
	{255,255,0,255},
	{255,255,255,255},
	{255,0,255,255},
	{0,255,0,255},
	{255,255,0,255},
	{255,255,255,255},
	{255,0,255,255}
};

std::vector<sAdvatekDevice*> foundDevices;
std::vector<std::pair<sAdvatekDevice*, sAdvatekDevice*>> syncDevices;
sAdvatekDevice* syncDevice;
sAdvatekDevice* syncDeviceVirt;

advatek_manager adv;
sImportOptions userImportOptions = sImportOptions();
sImportOptions virtualImportOptions = sImportOptions();

std::string adaptor_string = "None";
std::string json_device_string = "Select Device";
std::string vDeviceString = "New ...";
std::string result = "";
std::string vDeviceData = "";

loopVar s_loopVar;
updateRequest s_updateRequest;
AppLog applog;

bool SliderInt8(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
	return ImGui::SliderScalar(label, ImGuiDataType_U8, v, &v_min, &v_max, format, flags);
}

bool SliderInt16(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
	return ImGui::SliderScalar(label, ImGuiDataType_U16, v, &v_min, &v_max, format, flags);
}

void setupWindow(GLFWwindow*& window)
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
	s_loopVar.window_w = 800 * xscale;
	s_loopVar.window_h = 600 * yscale;
	s_loopVar.scale = std::max(xscale, yscale);

	int center_x = (mode->width / 2) - (s_loopVar.window_w / 2);
	int center_y = (mode->height / 2) - (s_loopVar.window_h / 2);

	// Create window with graphics context
	window = glfwCreateWindow(s_loopVar.window_w, s_loopVar.window_h, "Advatek Assistor", NULL, NULL);

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
	scaleToScreenDPI(io);

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
}

void scaleToScreenDPI(ImGuiIO& io)
{
	ImGui::GetStyle().ScaleAllSizes(s_loopVar.scale);
	ImFontConfig fc = ImFontConfig();
	fc.OversampleH = fc.OversampleV = s_loopVar.scale;
	fc.SizePixels = 13.f * s_loopVar.scale;
	io.Fonts->AddFontDefault(&fc);
}

void showResult(std::string& result) {
	if (result.empty()) return;
		
	ImGui::OpenPopup("Result");

	//Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f * s_loopVar.scale, 0.5f * s_loopVar.scale));
	//ImGui::SetNextWindowSize(ImVec2(400, 300));
	ImGui::SetNextWindowSizeConstraints(ImVec2(300.f * s_loopVar.scale, -1.f * s_loopVar.scale), ImVec2(INFINITY, -1.f * s_loopVar.scale));

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
		if (ImGui::Button("OK", ImVec2(120 * s_loopVar.scale, 0))) {
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
		s_updateRequest.poll = true;
	}
}

void colouredText(const char* txt, uint32_t color) {
	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::TextWrapped(txt);
	ImGui::PopStyleColor();
}

void importUI(sAdvatekDevice* device, sImportOptions& importOptions) {
	if (ImGui::BeginPopupModal("Import", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		// Read in devices
		pt::ptree pt_json_devices;
		std::vector<pt::ptree> loadedJsonDevices;
		std::vector<std::string> jsonDeviceNames;

		std::stringstream ss_json_devices;
		ss_json_devices << importOptions.json;
		pt::read_json(ss_json_devices, pt_json_devices);

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
			s_loopVar.pt_json_device = pt_json_devices;
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
					const bool is_selected = (s_loopVar.selectedNewImportIndex == n);
					if (ImGui::Selectable(jsonDeviceNames[n].c_str(), is_selected))
					{
						json_device_string = jsonDeviceNames[n];
						s_loopVar.pt_json_device = loadedJsonDevices[n];
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

		if (ImGui::Button("Import", ImVec2(120 * s_loopVar.scale, 0))) {
			std::stringstream jsonStringStream;
			write_json(jsonStringStream, s_loopVar.pt_json_device);
			importOptions.json = jsonStringStream.str();
			importOptions.userSet = true;
			ImGui::CloseCurrentPopup();
			s_loopVar.selectedNewImportIndex = -1;
			s_loopVar.pt_json_device.clear();
		}

		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120 * s_loopVar.scale, 0))) { ImGui::CloseCurrentPopup(); }
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
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f * s_loopVar.scale, 0.5f * s_loopVar.scale));

	importUI(device, userImportOptions);

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

std::stringstream Title;
bool b_setTest, testModeEnessColourOuputs = NULL;

void showDevices(std::vector<sAdvatekDevice*>& devices, bool isConnected) {

	ImGui::Spacing();

	for (int i = 0; i < devices.size(); i++) {
		bool deviceInRange = adv.ipInRange(adaptor_string, devices[i]);
		std::string modelName((char*)devices[i]->Model);
		modelName.append("          ").resize(19);
		modelName.append(" ");

		Title.str(std::string());
		Title.clear();
		Title << " " << modelName.c_str() << devices[i]->Firmware << "	";
		if (isConnected) {
			Title << ipString(devices[i]->CurrentIP);
		}
		else {
			Title << ipString(devices[i]->StaticIP);
		}
		Title.setf(std::ios::fixed, std::ios::floatfield);
		Title.precision(2);
		Title << "	" << "Temp: " << (float)devices[i]->Temperature * 0.1 << "	" << devices[i]->Nickname;
		Title << "###" << devices[i]->uid;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f * s_loopVar.scale, 8.f * s_loopVar.scale));
		bool node_open = ImGui::TreeNodeEx(Title.str().c_str(), ImGuiSelectableFlags_SpanAllColumns);
		ImGui::PopStyleVar();

		if (node_open)
		{
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
					s_updateRequest.clearVirtualDeviceIndex = i;
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
				ImGui::PushItemWidth(240 * s_loopVar.scale);
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
					ImGui::PushStyleColor(ImGuiCol_Text, COL_GREY);
					ImGui::Text("The IP address settings are not compatible with your adaptor settings.\nChange the IP settings on the device to access all settings.");
					ImGui::PopStyleColor();
					ImGui::Spacing();
				}

				ImGui::Text("Static IP Address:");

				ImGui::PushItemWidth(30 * s_loopVar.scale);

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

					ImGui::PushItemWidth(50 * s_loopVar.scale);

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

						delete[] tempReversed;
						delete[] tempEndUniverse;
						delete[] tempEndChannel;

					} // End Else/If Simple Config

					ImGui::PopItemWidth();

					ImGui::Spacing();
					ImGui::PushStyleColor(ImGuiCol_Text, COL_GREY);
					ImGui::Text("Note: Specified Maximum Pixels Per Output is %i", devices[i]->MaxPixPerOutput);
					ImGui::PopStyleColor();

					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("DMX512 Outputs"))
				{
					ImGui::PushItemWidth(50 * s_loopVar.scale);

					for (int DMXoutput = 0; DMXoutput < devices[i]->NumDMXOutputs; DMXoutput++) {
						ImGui::PushID(DMXoutput);
						devices[i]->TempDmxOutOn[DMXoutput] = (bool)devices[i]->DmxOutOn[DMXoutput];

						ImGui::Text("Output %i", DMXoutput + 1);
						ImGui::SameLine();

						if (ImGui::Checkbox("Enabled", &devices[i]->TempDmxOutOn[DMXoutput])) {
							devices[i]->DmxOutOn[DMXoutput] = (uint8_t)devices[i]->TempDmxOutOn[DMXoutput];
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
					ImGui::PushItemWidth(120 * s_loopVar.scale);
					ImGui::Combo("Pixel IC", &devices[i]->CurrentDriver, devices[i]->DriverNames, devices[i]->NumDrivers);
					ImGui::Combo("Clock Speed", &devices[i]->CurrentDriverSpeed, DriverSpeedsMhz, 12);
					bool tempExpanded = (bool)devices[i]->CurrentDriverExpanded;
					if (ImGui::Checkbox("Expanded Mode", &tempExpanded)) {
						devices[i]->CurrentDriverExpanded = (uint8_t)tempExpanded;
					}

					int tempAllColOrder = devices[i]->OutputColOrder[0];
					if (ImGui::Combo("RGB Order ##all", &tempAllColOrder, RGBW_Order, 24)) {
						for (int output = 0; output < devices[i]->NumOutputs * 0.5; output++) {
							devices[i]->OutputColOrder[output] = (uint8_t)tempAllColOrder;
						}
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
				if (isConnected) {
					if (ImGui::BeginTabItem("Test"))
					{
						ImGui::Spacing();
						ImGui::PushStyleColor(ImGuiCol_Text, COL_GREY);
						ImGui::Text("Prior to running test mode all other setings should be saved to controller. (Press 'Update Settings')");
						ImGui::PopStyleColor();
						ImGui::Spacing();

						b_setTest = false;

						ImGui::PushItemWidth(200 * s_loopVar.scale);

						if (ImGui::Combo("Set Test", &devices[i]->TestMode, TestModes, sizeof(TestModes) / sizeof(TestModes[0]))) {
							devices[i]->TestPixelNum = 0;
							s_loopVar.b_testPixelsReady = true;
							b_setTest = true;
						}

						ImGui::SameLine();

						// testAll
						if (ImGui::Checkbox("Test All", &adv.bTestAll)) {
							b_setTest = true;
						}

						if (devices[i]->ProtVer < 8 && devices[i]->TestMode == 8) {
							ImGui::Text("Device does not support single pixel output...");
							devices[i]->TestMode = 6;
						}

						if ((devices[i]->TestMode == 6) || (devices[i]->TestMode == 8)) {

							testModeEnessColourOuputs = devices[i]->testModeEnessColourOuputs;
							if (ImGui::Checkbox("ENESS Output Test", &testModeEnessColourOuputs)) {
								devices[i]->testModeEnessColourOuputs = (bool)testModeEnessColourOuputs;
							}

							if (devices[i]->testModeEnessColourOuputs) {
								// Cycle colours
								if (s_loopVar.currTime - s_loopVar.lastTime > s_loopVar.testCycleSpeed) {
									devices[i]->TestOutputNum = (devices[i]->TestOutputNum) % ((int)(devices[i]->NumOutputs * 0.5)) + 1;

									devices[i]->TestCols[0] = (int)(eness_colourcode_ouptput[devices[i]->TestOutputNum-1][0] * 255);
									devices[i]->TestCols[1] = (int)(eness_colourcode_ouptput[devices[i]->TestOutputNum-1][1] * 255);
									devices[i]->TestCols[2] = (int)(eness_colourcode_ouptput[devices[i]->TestOutputNum-1][2] * 255);
									devices[i]->TestCols[3] = (int)(eness_colourcode_ouptput[devices[i]->TestOutputNum-1][3] * 255);

									s_loopVar.lastTime = s_loopVar.currTime;
									b_setTest = true;
								}
							} else {
								if (ImGui::ColorEdit4("Test Colour", devices[i]->tempTestCols)) {
									devices[i]->TestCols[0] = (int)(devices[i]->tempTestCols[0] * 255);
									devices[i]->TestCols[1] = (int)(devices[i]->tempTestCols[1] * 255);
									devices[i]->TestCols[2] = (int)(devices[i]->tempTestCols[2] * 255);
									devices[i]->TestCols[3] = (int)(devices[i]->tempTestCols[3] * 255);
									b_setTest = true;
								}
							}

							if (devices[i]->ProtVer > 7) {

								if (devices[i]->testModeCycleOuputs || devices[i]->testModeCyclePixels) {

									if (s_loopVar.currTime - s_loopVar.lastTime > s_loopVar.testCycleSpeed) {
										if (devices[i]->TestMode == 8) {
											// Set Pixel
											devices[i]->TestPixelNum = (devices[i]->TestPixelNum) % ((int)(devices[i]->OutputPixels[(int)devices[i]->TestOutputNum - 1])) + 1;

											if (devices[i]->TestPixelNum == 1) {
												s_loopVar.b_testPixelsReady = true;
											}
											else {
												s_loopVar.b_testPixelsReady = false;
											}
										}
										if (devices[i]->testModeCycleOuputs && s_loopVar.b_testPixelsReady) {
											devices[i]->TestOutputNum = (devices[i]->TestOutputNum) % ((int)(devices[i]->NumOutputs * 0.5)) + 1;
										}
										s_loopVar.lastTime = s_loopVar.currTime;
										b_setTest = true;
									}
								}

								ImGui::Checkbox("Cycle Outputs", &devices[i]->testModeCycleOuputs);
								ImGui::Checkbox("Cycle Pixels", &devices[i]->testModeCyclePixels);
								ImGui::SliderFloat("Speed", &s_loopVar.testCycleSpeed, 5.0, 0.01, "%.01f");

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
						ImGui::PushItemWidth(25 * s_loopVar.scale);
						ImGui::SameLine(); ImGui::InputScalar("###Mac01", ImGuiDataType_U8, &devices[i]->Mac[0], 0, 0, "%02X");
						ImGui::SameLine(); ImGui::InputScalar("###Mac02", ImGuiDataType_U8, &devices[i]->Mac[1], 0, 0, "%02X");
						ImGui::SameLine(); ImGui::InputScalar("###Mac03", ImGuiDataType_U8, &devices[i]->Mac[2], 0, 0, "%02X");
						ImGui::SameLine(); ImGui::InputScalar("###Mac04", ImGuiDataType_U8, &devices[i]->Mac[3], 0, 0, "%02X");
						ImGui::SameLine(); ImGui::InputScalar("###Mac05", ImGuiDataType_U8, &devices[i]->Mac[4], 0, 0, "%02X");
						ImGui::SameLine(); ImGui::InputScalar("###Mac06", ImGuiDataType_U8, &devices[i]->Mac[5], 0, 0, "%02X");
						ImGui::PopItemWidth();
					}

					ImGui::Text("Protocol Version: %s", std::to_string(devices[i]->ProtVer).c_str());

					ImGui::PushItemWidth(200 * s_loopVar.scale);
					char sName[40];
					memcpy(sName, devices[i]->Nickname, 40);
					if (ImGui::InputText("Nickname", sName, 40)) {
						memset(devices[i]->Nickname, 0, 40);
						strcpy(devices[i]->Nickname, sName);
					}
					ImGui::PopItemWidth();

					ImGui::PushItemWidth(30 * s_loopVar.scale);

					ImGui::InputScalar("Fan Control On Temp", ImGuiDataType_U8, &devices[i]->MaxTargetTemp, 0, 0, 0);
					ImGui::PopItemWidth();

					if (isConnected) {
						int* tempVoltage = new int[devices[i]->NumBanks];

						for (int bank = 0; bank < devices[i]->NumBanks; bank++) {
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
					s_updateRequest.poll = true;
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

void showSyncDevice(sAdvatekDevice* vdevice, bool& canSyncAll, bool& inSyncAll)
{
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	colouredText("(Virtual Device)", COL_GREY);  ImGui::SameLine();
	ImGui::Text(ipString(vdevice->StaticIP).c_str()); ImGui::SameLine();
	colouredText(vdevice->Nickname, COL_LIGHTGREY);

	bool deviceInRange = adv.ipInRange(adaptor_string, vdevice);
	if (!deviceInRange) {
		colouredText("Device IP address settings not compatible with your adaptor settings.", COL_RED);
		colouredText("Change the IP settings of your adaptor.", COL_RED);
		canSyncAll = false;
		return;
	}

	foundDevices.clear();

	switch (s_loopVar.current_sync_type) {
	case 0: // Match Static IP
		foundDevices = adv.getDevicesWithStaticIP(adv.connectedDevices, ipString(vdevice->StaticIP));
		if (foundDevices.size() != 1) {
			if (foundDevices.size()) {
				colouredText(std::string("Multiple connected devices with static IP ").append(ipString(vdevice->StaticIP)).c_str(), COL_RED);
			}
			else {
				colouredText(std::string("No connected device with static IP ").append(ipString(vdevice->StaticIP)).c_str(), COL_RED);
			}
			canSyncAll = false;
			return;
		}
		break;
	case 1: // Match Nickname
		foundDevices = adv.getDevicesWithNickname(adv.connectedDevices, std::string(vdevice->Nickname));
		if (foundDevices.size() != 1) {
			if (foundDevices.size()) {
				colouredText(std::string("Multiple connected devices with Nickname ").append(vdevice->Nickname).c_str(), COL_RED);
			}
			else {
				colouredText(std::string("No connected device with Nickname ").append(vdevice->Nickname).c_str(), COL_RED);
			}
			canSyncAll = false;
			return;
		}
		break;
	case 2: // Match MAC
		foundDevices = adv.getDevicesWithMac(adv.connectedDevices, macString(vdevice->Mac));
		if (foundDevices.size() != 1) {
			if (foundDevices.size()) {
				// This should never happen :)
				colouredText(std::string("Whoah!! Multiple connected devices with MAC address ").append(macString(vdevice->Mac)).c_str(), COL_RED);
			}
			else {
				colouredText(std::string("No connected device with MAC address ").append(macString(vdevice->Mac)).c_str(), COL_RED);
			}
			canSyncAll = false;
			return;
		}
		break;
	default:
		return;
		break;
	}

	if (adv.deviceCompatible(vdevice, foundDevices[0])) {
		if (adv.devicesInSync(vdevice, foundDevices[0])) {
			colouredText("Device in sync ...", COL_GREEN);
		}
		else {
			inSyncAll = false;
			syncDevices.emplace_back(vdevice, foundDevices[0]);
			if (ImGui::Button("Update Connected Device"))
			{
				if (syncDevice) delete[] syncDevice;
				if (syncDeviceVirt) delete[] syncDeviceVirt;
				syncDevice = foundDevices[0];
				syncDeviceVirt = vdevice;
				s_updateRequest.syncDevice = true;
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

std::stringstream tabTitleConnected;
std::stringstream tabTitleVirtual;
bool canSyncAll, inSyncAll = NULL;

void showWindow(GLFWwindow*& window)
{
	{
		ImGuiWindowFlags window_flags = 0;
		window_flags |= ImGuiWindowFlags_NoTitleBar;
		window_flags |= ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoResize;
		window_flags |= ImGuiWindowFlags_NoCollapse;

		ImGui::Begin("Advatek Assistor", NULL, window_flags);

		showResult(result);

		if (ImGui::Button("Refresh Adaptors"))
		{
			s_updateRequest.refreshAdaptors = true;
		} ImGui::SameLine();

		ImGui::PushItemWidth(130 * s_loopVar.scale);

		if (ImGui::BeginCombo("###Adaptor", adaptor_string.c_str(), 0))
		{
			for (int n = 0; n < adv.networkAdaptors.size(); n++)
			{
				const bool is_selected = (adv.currentAdaptor == n);
				if (ImGui::Selectable(adv.networkAdaptors[n].c_str(), is_selected))
				{
					adaptor_string = adv.networkAdaptors[n];
					adv.setCurrentAdaptor(n);
					applog.AddLog(("[INFO] Set network adaptor to " + adaptor_string + "...\n").c_str());
					s_updateRequest.poll = true;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Spacing();

		if (ImGui::BeginTabBar("Devices")) {
			tabTitleConnected.str(std::string());
			tabTitleVirtual.str(std::string());
			tabTitleConnected.clear();
			tabTitleVirtual.clear();
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
					s_updateRequest.poll = true;
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
						s_updateRequest.connectedDevicesToVirtualDevices = true;
					}
				}

				if (adv.connectedDevices.size() > 1) {
					ImGui::SameLine();
					ImGui::Combo("###SortConnectedDevices", &adv.sortTypeConnected, adv.SortTypes, IM_ARRAYSIZE(adv.SortTypes));
				}
				ImGui::Spacing();

				showDevices(adv.connectedDevices, true);

				// END Connected Devices
				ImGui::EndTabItem();
			}

			// -------------------------------------- START SYNC
			if (ImGui::BeginTabItem("<-"))
			{
				ImGui::Spacing();
				ImGui::PushItemWidth(182 * s_loopVar.scale);
				ImGui::Combo("###SyncTypes", &s_loopVar.current_sync_type, SyncTypes, IM_ARRAYSIZE(SyncTypes));
				ImGui::PopItemWidth();
				ImGui::SameLine();

				canSyncAll = true;
				inSyncAll = true;

				// Check static IP of all virtual devices for conflict
				// In name mode check for dupplicate names

				syncDevices.clear();

				for (int i = 0; i < adv.virtualDevices.size(); i++) {
					ImGui::PushID(i);
					showSyncDevice(adv.virtualDevices[i], canSyncAll, inSyncAll);
					ImGui::PopID();
				}

				ImGui::Spacing();

				if (canSyncAll && !inSyncAll) {
					ImGui::Separator();
					ImGui::Spacing();
					if (ImGui::Button("Update All"))
					{
						s_updateRequest.syncVirtualDevices = true;
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
							s_updateRequest.newVirtualDevice = true;
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

							s_updateRequest.newVirtualDevice = true;
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
						s_updateRequest.pasteToNewVirtualDevice = true;
					}
				}

				if (adv.virtualDevices.size() >= 1) {
					ImGui::SameLine();
					if (ImGui::Button("Clear"))
					{
						s_updateRequest.clearVirtualDevices = true;
					}
				}

				if (adv.virtualDevices.size() > 1) {
					ImGui::SameLine();
					ImGui::Combo("###SortVirtualDevices", &adv.sortTypeVirtual, adv.SortTypes, IM_ARRAYSIZE(adv.SortTypes) - 2);
				}

				ImGui::Spacing();

				showDevices(adv.virtualDevices, false);

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Separator();

		applog.Draw("Advatek Assistor");

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Spacing();
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
	if (s_updateRequest.refreshAdaptors) {
		applog.AddLog("[INFO] Refreshing network adaptors...\n");
		s_updateRequest.refreshAdaptors = false;
		adv.refreshAdaptors();
		if (adv.networkAdaptors.size() > 0) {
			adaptor_string = adv.networkAdaptors[0];
			adv.poll();
			applog.AddLog(("[INFO] Polling using network adaptor " + adaptor_string + "...\n").c_str());
			s_loopVar.lastPoll = s_loopVar.currTime;
		}
		else {
			adaptor_string = "None";
		}
	}

	if (s_updateRequest.poll) {
		adv.poll();
		applog.AddLog(("[INFO] Polling using network adaptor " + adaptor_string + "...\n").c_str());
		s_updateRequest.poll = false;
	}

	if (s_updateRequest.newVirtualDevice) {
		adv.addVirtualDevice(virtualImportOptions);
		applog.AddLog("[INFO] Created new Virtual Device.\n");
		s_updateRequest.newVirtualDevice = false;
	}

	if (s_updateRequest.pasteToNewVirtualDevice) {
		adv.pasteToNewVirtualDevice();
		applog.AddLog("[INFO] Pasted data into new virtual device.\n");
		s_updateRequest.pasteToNewVirtualDevice = false;
	}

	if (s_updateRequest.clearVirtualDevices) {
		adv.clearDevices(adv.virtualDevices);
		s_updateRequest.clearVirtualDevices = false;
		applog.AddLog("[INFO] Cleared all virtual devices.\n");
	}

	if (s_updateRequest.clearVirtualDeviceIndex >= 0) {
		applog.AddLog(("[INFO] Removing Virtual Device " + ipString(adv.virtualDevices[s_updateRequest.clearVirtualDeviceIndex]->StaticIP).append(" ").append(adv.virtualDevices[s_updateRequest.clearVirtualDeviceIndex]->Nickname).append("\n")).c_str());
		adv.virtualDevices.erase(adv.virtualDevices.begin() + s_updateRequest.clearVirtualDeviceIndex);
		s_updateRequest.clearVirtualDeviceIndex = -1;
	}

	if (s_updateRequest.connectedDevicesToVirtualDevices) {
		applog.AddLog("[INFO] Copying Connected Devices to Virtual Devices...");
		// Show loading bar here?
		for (auto device : adv.connectedDevices) {
			boost::property_tree::ptree advatek_device;
			adv.getJSON(device, advatek_device);
			sImportOptions conImportOptions = sImportOptions();
			conImportOptions.init = true;
			adv.addVirtualDevice(advatek_device, conImportOptions);
		}
		s_updateRequest.connectedDevicesToVirtualDevices = false;
	}

	if (s_updateRequest.syncDevice) {
		adv.updateConnectedDevice(syncDeviceVirt, syncDevice);
		adv.removeConnectedDevice(macString(syncDevice->Mac));
		s_updateRequest.syncDevice = false;
	}

	if (s_updateRequest.syncVirtualDevices) {
		for (int i = 0; i < syncDevices.size(); i++) {
			adv.updateConnectedDevice(syncDevices[i].first, syncDevices[i].second);
			adv.removeConnectedDevice(macString(syncDevices[i].second->Mac));
		}
		s_updateRequest.syncVirtualDevices = false;
	}
}

AppLog::AppLog()
{
	AutoScroll = true;
	Clear();
}

void AppLog::Clear()
{
	Buf.clear();
	LineOffsets.clear();
	LineOffsets.push_back(0);
}

void AppLog::AddLog(const char* fmt, ...) IM_FMTARGS(2)
{
	int old_size = Buf.size();
	va_list args;
	va_start(args, fmt);
	Buf.appendfv(fmt, args);
	va_end(args);
	for (int new_size = Buf.size(); old_size < new_size; old_size++)
		if (Buf[old_size] == '\n')
			LineOffsets.push_back(old_size + 1);
}

void AppLog::Draw(const char* title, bool* p_open /*= NULL*/)
{
	if (!ImGui::Begin(title, p_open))
	{
		ImGui::End();
		return;
	}

	// Options menu
	//if (ImGui::BeginPopup("Options"))
	//{
	//	ImGui::Checkbox("Auto-scroll", &AutoScroll);
	//	ImGui::EndPopup();
	//}

	// Main window
	//if (ImGui::Button("Options"))
	//	ImGui::OpenPopup("Options");
	//ImGui::SameLine();

	ImGui::Spacing();

	//bool clear = ImGui::Button("Clear", ImVec2(50 * s_loopVar.scale,0));
	//ImGui::SameLine();
	bool copy = ImGui::Button("Copy", ImVec2(50 * s_loopVar.scale, 0));
	ImGui::SameLine();
	ImGui::PushItemWidth(130 * s_loopVar.scale);
	Filter.Draw("Filter");
	ImGui::PopItemWidth();

	//ImGui::Separator();
	ImGui::Spacing();
	ImGui::BeginChild("scrolling", ImVec2(0, 50 * s_loopVar.scale), false, ImGuiWindowFlags_HorizontalScrollbar);

	//if (clear)
	//	Clear();
	if (copy)
		ImGui::LogToClipboard();

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	ImGui::PushStyleColor(ImGuiCol_Text, COL_GREY);

	const char* buf = Buf.begin();
	const char* buf_end = Buf.end();
	if (Filter.IsActive())
	{
		for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
		{
			const char* line_start = buf + LineOffsets[line_no];
			const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
			if (Filter.PassFilter(line_start, line_end))
				ImGui::TextUnformatted(line_start, line_end);
		}
	}
	else
	{
		ImGuiListClipper clipper;
		clipper.Begin(LineOffsets.Size);
		while (clipper.Step())
		{
			for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
			{
				const char* line_start = buf + LineOffsets[line_no];
				const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
				ImGui::TextUnformatted(line_start, line_end);
			}
		}
		clipper.End();
	}

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();

	if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.0);

	ImGui::EndChild();
	ImGui::End();
}
