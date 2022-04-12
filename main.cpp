// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#if defined(__arm__)
#define IMGUI_IMPL_OPENGL_ES2 // RPI
#endif

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "portable-file-dialogs.h"

#include "advatek_assistor.h"
#include "standard_json_config.h"

#define Version "1.1.0"

double currTime = 0;
double lastPoll = 0;
double lastTime = 0;

float rePollTime = 3;
float testCycleSpeed = 0.5;
int b_testPixelsReady = true;

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

bool SliderInt8(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0)
{
	return ImGui::SliderScalar(label, ImGuiDataType_U8, v, &v_min, &v_max, format, flags);
}

bool SliderInt16(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0)
{
	return ImGui::SliderScalar(label, ImGuiDataType_U16, v, &v_min, &v_max, format, flags);
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

advatek_manager adv;
sImportOptions importOptions = sImportOptions();

int b_pollRequest = 0;
int b_refreshAdaptorsRequest = 0;
int b_newVirtualDeviceRequest = 0;
int b_pasteToNewVirtualDevice = 0;
int b_clearVirtualDevicesRequest = 0;
int iClearVirtualDeviceID = -1;
int b_vDevicePath = false;

static std::string adaptor_string = "No Adaptors Found";
static std::string vDeviceString = "New ...";
static std::string result = "";
static std::string vDeviceData = "";

bool logOpen = true;

struct AppLog {
	ImGuiTextBuffer     Buf;
	ImGuiTextFilter     Filter;
	ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
	bool                AutoScroll;  // Keep scrolling if already at the bottom.

	AppLog() {
		AutoScroll = true;
		Clear();
	}

	void Clear() {
		Buf.clear();
		LineOffsets.clear();
		LineOffsets.push_back(0);
	}

	void AddLog(const char* fmt, ...) IM_FMTARGS(2) {
		int old_size = Buf.size();
		va_list args;
		va_start(args, fmt);
		Buf.appendfv(fmt, args);
		va_end(args);
		for (int new_size = Buf.size(); old_size < new_size; old_size++)
			if (Buf[old_size] == '\n')
				LineOffsets.push_back(old_size + 1);
	}

	void Draw(const char* title, bool* p_open = NULL) {
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

		bool clear = ImGui::Button("Clear", ImVec2(50,0));
		ImGui::SameLine();
		bool copy = ImGui::Button("Copy", ImVec2(50, 0));
		ImGui::SameLine();
		ImGui::PushItemWidth(130);
		Filter.Draw("Filter");
		ImGui::PopItemWidth();

		//ImGui::Separator();
		ImGui::Spacing();
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		if (clear)
			Clear();
		if (copy)
			ImGui::LogToClipboard();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

		auto txtCol = IM_COL32(120, 120, 120, 255);
		ImGui::PushStyleColor(ImGuiCol_Text, txtCol);

		const char* buf = Buf.begin();
		const char* buf_end = Buf.end();
		if (Filter.IsActive())
		{
			// In this example we don't use the clipper when Filter is enabled.
			// This is because we don't have a random access on the result on our filter.
			// A real application processing logs with ten of thousands of entries may want to store the result of
			// search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
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
			// The simplest and easy way to display the entire buffer:
			//   ImGui::TextUnformatted(buf_begin, buf_end);
			// And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
			// to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
			// within the visible area.
			// If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
			// on your side is recommended. Using ImGuiListClipper requires
			// - A) random access into your data
			// - B) items all being the  same height,
			// both of which we can handle since we an array pointing to the beginning of each line of text.
			// When using the filter (in the block of code above) we don't have random access into the data to display
			// anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
			// it possible (and would be recommended if you want to search through tens of thousands of entries).
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
			ImGui::SetScrollHereY(1.0f);

		ImGui::EndChild();
		ImGui::End();
	}
};

static AppLog applog;

void showResult(std::string& result) {
	if (result.empty() == false)
		ImGui::OpenPopup("Result");

	//Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	//ImGui::SetNextWindowSize(ImVec2(400, 300));
	ImGui::SetNextWindowSizeConstraints(ImVec2(300.f, -1.f), ImVec2(INFINITY, -1.f));

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

		auto txtCol = IM_COL32(80, 80, 80, 255);
		ImGui::PushStyleColor(ImGuiCol_Text, txtCol);
		ImGui::TextWrapped("Please click the Update Network or Update Settings to save these changes to the controller.");
		ImGui::PopStyleColor();

		ImGui::Spacing();
		if (ImGui::Button("OK", ImVec2(120, 0))) {
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

void button_import_export_JSON(sAdvatekDevice *device) {
	
	if (ImGui::Button("Copy"))
	{
		adv.copyToMemoryDevice(device);
		applog.AddLog("[INFO] Copied data from %s (%s)\n", device->Nickname, ipString(device->CurrentIP).c_str());
	}

	ImGui::SameLine();

	if (adv.memoryDevices.size() == 1) {
		if (ImGui::Button("Paste"))
		{
			adv.pasteFromMemoryDevice(device);
			applog.AddLog("[INFO] Pasted data to %s (%s)\n", device->Nickname, ipString(device->CurrentIP).c_str());
		}
	}
	
	ImGui::SameLine();

	if (ImGui::Button("Import JSON"))
		ImGui::OpenPopup("Import");

	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Import", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("What needs importing?");
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		ImGui::Checkbox("Network", &importOptions.network);
		ImGui::Checkbox("Ethernet Control (Mapping)", &importOptions.ethernet_control);
		ImGui::Checkbox("DMX512 Outputs", &importOptions.dmx_outputs);
		ImGui::Checkbox("LED Settings", &importOptions.led_settings);
		ImGui::Checkbox("Nickname", &importOptions.nickname);
		ImGui::Checkbox("Fan On Temp", &importOptions.fan_on_temp);

		ImGui::PopStyleVar();
		ImGui::Spacing();

		if (ImGui::Button("Import", ImVec2(120, 0))) { 
			ImGui::CloseCurrentPopup(); 
			auto path = pfd::open_file("Select a file", ".", { "JSON Files", "*.json *.JSON" }).result();
			if (!path.empty()) {
				applog.AddLog("[INFO] Loading JSON file from %s\n", path.at(0).c_str());
				//result = adv.importJSON(adv.connectedDevices[d], path.at(0), importOptions);
				result = adv.importJSON(device, path.at(0), importOptions);
			}
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
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

void showDevices(std::vector<sAdvatekDevice*> &devices, bool isConnected) {
	for (uint8_t i = 0; i < devices.size(); i++) {
		std::stringstream Title;
		Title << devices[i]->Model << "	" << devices[i]->Firmware << "	" << ipString(devices[i]->CurrentIP) << "		" << "Temp: " << (float)devices[i]->Temperature*0.1 << "		" << devices[i]->Nickname;
		Title << "###" << macString(devices[i]->Mac) << i;
		bool node_open = ImGui::TreeNodeEx(Title.str().c_str(), ImGuiSelectableFlags_SpanAllColumns);

		if (node_open)
		{
			ImGui::Columns(1);
			ImGui::Spacing();

			if (isConnected) {
				if (ImGui::Button("ID"))
				{
					adv.identifyDevice(i, 20);
				}
				ImGui::SameLine();
			} else {
				if (ImGui::Button("Delete"))
				{
					iClearVirtualDeviceID = i;
				}
				ImGui::SameLine();
			}

			button_import_export_JSON(devices[i]);

			ImGui::Separator();
			ImGui::Spacing();

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

				if (isConnected) {
					if (ImGui::Button("Update Network"))
					{
						adv.bc_networkConfig(i);
						b_pollRequest = true;
					}
				}

				ImGui::EndTabItem();
			}
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

				ImGui::PushItemWidth(50);

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

					bool * tempReversed = new bool[devices[i]->NumOutputs];
					uint16_t * tempEndUniverse = new uint16_t[devices[i]->NumOutputs];
					uint16_t * tempEndChannel = new uint16_t[devices[i]->NumOutputs];

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

						for (int output = 0; output < devices[i]->NumOutputs*0.5; output++)
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

				if (isConnected) {
					button_update_controller_settings(i);
				}

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("DMX512 Outputs"))
			{
				bool * tempDMXOffOn = new bool[devices[i]->NumDMXOutputs];

				ImGui::PushItemWidth(50);

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

				if (isConnected) {
					button_update_controller_settings(i);
				}

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

				int tempAllColOrder = devices[i]->OutputColOrder[0];
				if (ImGui::Combo("RGB Order ##all", &tempAllColOrder, advatek_manager::RGBW_Order, 24)) {
					for (uint8_t output = 0; output < devices[i]->NumOutputs*0.5; output++) {
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

				if (isConnected) {
					button_update_controller_settings(i);
				}

				ImGui::EndTabItem();
			}
			if (isConnected) {
				if (ImGui::BeginTabItem("Test"))
				{
					ImGui::Text("Prior to running test mode all other setings should be saved to controller. (Press 'Update Settings')");

					ImGui::PushItemWidth(200);

					if (ImGui::Combo("Set Test", &devices[i]->TestMode, TestModes, 9)) {
						devices[i]->TestPixelNum = 0;
						b_testPixelsReady = true;
						adv.setTest(i);
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
							adv.setTest(i);
						}

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
									devices[i]->TestOutputNum = (devices[i]->TestOutputNum) % ((int)(devices[i]->NumOutputs*0.5)) + 1;
								}
								lastTime = currTime;
								adv.setTest(i);
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

						//ImGui::SameLine();

						if (devices[i]->TestMode == 8) {
							bool testModeCyclePixels = devices[i]->testModeCyclePixels;
							if (ImGui::Checkbox("Cycle Pixels", &testModeCyclePixels)) {
								devices[i]->testModeCyclePixels = (bool)testModeCyclePixels;
							}
						}

						//ImGui::PushItemWidth(80);
						ImGui::SliderFloat("Speed", &testCycleSpeed, 5.0, 0.01, "%.01f");
						//ImGui::PopItemWidth();
					}

					//ImGui::PopItemWidth();

					//ImGui::PushItemWidth(100);
					//ImGui::Text("Output (All 0)"); ImGui::SameLine();
					if (SliderInt8("Output (All 0)", (int*)&devices[i]->TestOutputNum, 0, (devices[i]->NumOutputs*0.5))) {
						adv.setTest(i);
					}

					if (devices[i]->TestMode == 8) {
						//ImGui::Text("Pixels (All 0)"); ImGui::SameLine();
						uint16_t TestPixelNum = devices[i]->TestPixelNum;
						if (SliderInt16("Pixels (All 0)", (int*)&TestPixelNum, 0, (devices[i]->OutputPixels[(int)devices[i]->TestOutputNum - 1]))) {
							adv.setTest(i);
						}
					}

					ImGui::PopItemWidth();

					button_update_controller_settings(i);

					ImGui::EndTabItem();
				} // End Test Tab

			}
			
			if (ImGui::BeginTabItem("Misc"))
			{
				if (isConnected) {
					ImGui::Text("MAC: %s", macString(devices[i]->Mac).c_str());
				}

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

				if (isConnected) {
					int * tempVoltage = new int[devices[i]->NumBanks];

					for (uint8_t bank = 0; bank < devices[i]->NumBanks; bank++) {
						ImGui::PushID(bank);
						ImGui::Text("Bank %i: %.2f V", bank + 1, ((float)devices[i]->VoltageBanks[bank] / 10.f));
						ImGui::PopID();
					}

					button_update_controller_settings(i);
				}

				ImGui::EndTabItem();

			}
			ImGui::EndTabBar();
			ImGui::Spacing();
			ImGui::TreePop();		
		}
	}
}

#ifndef DEBUG
#pragma comment(linker, "/SUBSYSTEM:Windows /ENTRY:mainCRTStartup")
#endif

int main(int, char**)
{
	applog.AddLog("[INFO] Advatek Assistor v%s\n", Version);
	adv.refreshAdaptors();

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

	const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

	int window_w = 800;
	int window_h = 600;

	int centerx = (mode->width / 2) - (window_w / 2);
	int centery = (mode->height / 2) - (window_h / 2);

	// Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(window_w, window_h, "Advatek Assistor", NULL, NULL);

	glfwSetWindowPos(window, centerx, centery);

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
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsClassic();

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

	currTime = ImGui::GetTime();
	if (adv.networkAdaptors.size() > 0) {
		adaptor_string = adv.networkAdaptors[0];
		adv.poll();
		applog.AddLog(("[INFO] Polling using network adaptor " + adaptor_string + " ...\n").c_str());
		lastPoll = currTime;
	}

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
		currTime = ImGui::GetTime();
		adv.listen();

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

		glfwGetWindowSize(window, &window_w, &window_h);
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
		
		if ( (adv.connectedDevices.size() == 0) && (currTime - lastPoll > rePollTime)) {
			adv.poll();
			applog.AddLog(("[INFO] Polling using network adaptor " + adaptor_string + " ...\n").c_str());
			lastPoll = currTime;
		}
		
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

			ImGui::Begin("Advatek Assistor", NULL, window_flags);

			if (ImGui::Button("Refresh Adaptors"))
			{
				b_refreshAdaptorsRequest = true;
			} ImGui::SameLine();

			ImGui::PushItemWidth(130);

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
				ImGui::PopItemWidth();
			}
			ImGui::Spacing();
			// Connected Devices && Virtual Devices
			if (ImGui::BeginTabBar("Devices")) {
				if (ImGui::BeginTabItem("Connected Devices"))
				{
					ImGui::Spacing();
					if (ImGui::Button("Search"))
					{
						b_pollRequest = true;
					} ImGui::SameLine();

					ImGui::Text("%li Device(s) Connected", adv.connectedDevices.size());

					showResult(result);

					ImGui::Spacing();

					showDevices(adv.connectedDevices, true);

					// END Connected Devices
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Virtual Devices"))
				{
					ImGui::Spacing();

					if (ImGui::BeginCombo("###NewVirtualDevice", vDeviceString.c_str(), 0))
					{

						for(const auto& jsonData:JSONControllers)
						{
							const bool is_selected = (vDeviceString.c_str() == jsonData[0].c_str());
							if (ImGui::Selectable(jsonData[0].c_str(), is_selected))
							{
								vDeviceString = jsonData[0];
								vDeviceData = jsonData[1];
								b_vDevicePath = false;
								b_newVirtualDeviceRequest = true;
							}
						}
						ImGui::EndCombo();
						ImGui::PopItemWidth();
					}

					ImGui::SameLine();

					if (ImGui::Button("Import JSON")) {
						auto path = pfd::open_file("Select a file", ".", { "JSON Files", "*.json *.JSON" }).result();
						if (!path.empty()) {
							applog.AddLog("[INFO] Loading JSON file from %s\n", path.at(0).c_str());
							vDeviceData = path.at(0);
							b_vDevicePath = true;
							b_newVirtualDeviceRequest = true;
						}
					}

					ImGui::SameLine();

					if (adv.memoryDevices.size() == 1) {
						if (ImGui::Button("Paste"))
						{
							b_pasteToNewVirtualDevice = true;
						}
					}

					ImGui::SameLine();

					if (ImGui::Button("Clear"))
					{
						b_clearVirtualDevicesRequest = true;
					} ImGui::SameLine();

					ImGui::Text("%li Device(s)", adv.virtualDevices.size());

					ImGui::Spacing();

					showDevices(adv.virtualDevices, false);

					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}

			ImGui::Spacing();
			ImGui::Separator();

			applog.Draw("Advatek Assistor", &logOpen);

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
			adv.refreshAdaptors();
			b_refreshAdaptorsRequest = false;
		}

		if (b_pollRequest) {
			adv.poll();
			b_pollRequest = false;
		}

		if (b_newVirtualDeviceRequest) {
			adv.addVirtualDevice(vDeviceData, b_vDevicePath);
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

    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
