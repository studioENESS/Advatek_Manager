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

#define Version "1.3.1"

namespace pt = boost::property_tree;

bool SliderInt8(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);

bool SliderInt16(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);

bool SliderInt8(const char* label, uint8_t* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);

bool SliderInt16(const char* label, uint16_t* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);

struct updateRequest {
	int poll = 0;
	int refreshAdaptors = 0;
	int newVirtualDevice = 0;
	int pasteToNewVirtualDevice = 0;
	int clearVirtualDevices = 0;
	int connectedDevicesToVirtualDevices = 0;
	int clearVirtualDeviceIndex = -1;
	int clearConnectedDeviceIndex = -1;
};

struct myTabBarFlags {
	ImGuiTabItemFlags network  = ImGuiTabItemFlags_None;
	ImGuiTabItemFlags ethernet = ImGuiTabItemFlags_None;
	ImGuiTabItemFlags dmx512   = ImGuiTabItemFlags_None;
	ImGuiTabItemFlags leds     = ImGuiTabItemFlags_None;
	ImGuiTabItemFlags test     = ImGuiTabItemFlags_None;
	ImGuiTabItemFlags misc     = ImGuiTabItemFlags_None;

	void select_network();
	void select_ethernet();
	void select_dmx512();
	void select_leds();
	void select_test();
	void select_misc();

	void Clear();
};

struct loopVar {
	bool logOpen = true;
	bool b_testPixelsReady = true;

	int window_w, window_h;
	int open_action = -1;
	int selectedNewImportIndex = -1;
	int current_sync_type = 0;

	double currTime = 0;
	double lastTime = 0;
	double lastPoll = 0;

	float rePollTime = 3;
	float testCycleSpeed = 0.5;
	float scale = 1;

	pt::ptree pt_json_device;
};

struct AppLog {
	ImGuiTextBuffer     Buf;
	ImGuiTextFilter     Filter;
	ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
	bool                AutoScroll;  // Keep scrolling if already at the bottom.

	AppLog();

	void Clear();

	void AddLog(const char* fmt, ...);

	void Draw(const char* title, bool* p_open = NULL);
};

extern myTabBarFlags s_myTabBarFlags;
extern loopVar s_loopVar;
extern AppLog applog;

extern float eness_colourcode_ouptput[16][4];

extern uint32_t COL_GREY, COL_LIGHTGREY, COL_GREEN, COL_RED;

extern std::string adaptor_string, json_device_string, vDeviceString, result, vDeviceData;

extern std::vector<sAdvatekDevice*> foundDevices;
extern std::vector<std::pair<sAdvatekDevice*, sAdvatekDevice*>> syncDevices;
extern advatek_manager adv;

extern sImportOptions userImportOptions, virtualImportOptions;

void setupWindow(GLFWwindow*& window);

void scaleToScreenDPI(ImGuiIO& io);

void showResult(std::string& result);

void button_update_controller_settings(sAdvatekDevice* device);
void button_open_close_all();

void colouredText(const char* txt, uint32_t color);

void importUI(sAdvatekDevice* device, sImportOptions& importOptions);

void button_import_export_JSON(sAdvatekDevice* device);

void showDevices(std::vector<sAdvatekDevice*>& devices, bool isConnected);

void showSyncDevice(sAdvatekDevice* vdevice, bool& canSyncAll, bool& inSyncAll);

void showWindow(GLFWwindow*& window);

void processUpdateRequests();