#pragma once

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
#include "advatek_assistor.h"
#include "portable-file-dialogs.h"

namespace pt = boost::property_tree;
extern pt::ptree pt_json_device;

extern uint32_t COL_GREY, COL_LIGHTGREY, COL_GREEN, COL_RED;

extern std::vector<sAdvatekDevice*> foundDevices;
extern std::vector<std::pair<sAdvatekDevice*, sAdvatekDevice*>> syncDevices;
extern sAdvatekDevice* syncDevice;

extern advatek_manager adv;
extern sImportOptions userImportOptions, virtualImportOptions;

extern double currTime, lastPoll, lastSoftPoll, lastTime;
extern float rePollTime, testCycleSpeed, scale;
extern int b_testPixelsReady, b_pollRequest, b_refreshAdaptorsRequest, b_newVirtualDeviceRequest, b_pasteToNewVirtualDevice,
b_clearVirtualDevicesRequest, b_copyAllConnectedToVirtualRequest, iClearVirtualDeviceID, syncConnectedDeviceRequest, b_vDevicePath, current_json_device, current_sync_type, b_syncAllRequest;
extern bool logOpen;

extern float eness_colourcode_ouptput[4][4];

extern std::string adaptor_string, json_device_string, vDeviceString, result, vDeviceData;

bool SliderInt8(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);

bool SliderInt16(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);

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

	void Draw(const char* title, bool* p_open = NULL, float scale = 1) {
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

		//bool clear = ImGui::Button("Clear", ImVec2(50 * scale,0));
		//ImGui::SameLine();
		bool copy = ImGui::Button("Copy", ImVec2(50 * scale, 0));
		ImGui::SameLine();
		ImGui::PushItemWidth(130 * scale);
		Filter.Draw("Filter");
		ImGui::PopItemWidth();

		//ImGui::Separator();
		ImGui::Spacing();
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

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

extern AppLog applog;

void setupWindow(GLFWwindow*& window, int& window_w, int& window_h, float& scale);

void scaleToScreenDPI(float& scale, ImGuiIO& io);

void showResult(std::string& result, float scale);

void button_update_controller_settings(int i);

void colouredText(const char* txt, uint32_t color);

void importUI(sAdvatekDevice* device, sImportOptions& importOptions, float scale);

void button_import_export_JSON(sAdvatekDevice* device);

void showDevices(std::vector<sAdvatekDevice*>& devices, bool isConnected);

void showSyncDevice(const uint8_t& i, bool& canSyncAll, bool& inSyncAll, float scale);

void showWindow(GLFWwindow*& window, int window_w, int window_h, float scale);

void processUpdateRequests();