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

extern uint32_t COL_GREY, COL_LIGHTGREY, COL_GREEN, COL_RED;

extern std::vector<sAdvatekDevice*> foundDevices;
extern std::vector<std::pair<sAdvatekDevice*, sAdvatekDevice*>> syncDevices;
extern sAdvatekDevice* syncDevice;

extern advatek_manager adv;
extern sImportOptions userImportOptions, virtualImportOptions;

extern float eness_colourcode_ouptput[16][4];

extern std::string adaptor_string, json_device_string, vDeviceString, result, vDeviceData;

bool SliderInt8(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);

bool SliderInt16(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);

struct updateRequest {
	int poll = 0;
	int refreshAdaptors = 0;
	int newVirtualDevice = 0;
	int pasteToNewVirtualDevice = 0;
	int clearVirtualDevices = 0;
	int connectedDevicesToVirtualDevices = 0;
	int clearVirtualDeviceIndex = -1;
	int syncDevice = 0;
	int syncVirtualDevices = 0;
};

struct loopVar {
	int window_w, window_h;

	double currTime = 0;
	double lastTime = 0;
	double lastPoll = 0;

	float rePollTime = 3;
	float testCycleSpeed = 0.5;
	float scale = 1;

	bool logOpen = true;

	int selectedNewImportIndex = -1;
	int current_sync_type = 0;
	int b_testPixelsReady = true;

	pt::ptree pt_json_device;
};
extern loopVar s_loopVar;

struct AppLog {
	ImGuiTextBuffer     Buf;
	ImGuiTextFilter     Filter;
	ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
	bool                AutoScroll;  // Keep scrolling if already at the bottom.

	AppLog();

	void Clear();

	void AddLog(const char* fmt, ...) IM_FMTARGS(2);

	void Draw(const char* title, bool* p_open = NULL);
};
extern AppLog applog;

void setupWindow(GLFWwindow*& window);

void scaleToScreenDPI(ImGuiIO& io);

void showResult(std::string& result);

void button_update_controller_settings(int i);

void colouredText(const char* txt, uint32_t color);

void importUI(sAdvatekDevice* device, sImportOptions& importOptions);

void button_import_export_JSON(sAdvatekDevice* device);

void showDevices(std::vector<sAdvatekDevice*>& devices, bool isConnected);

void showSyncDevice(sAdvatekDevice* vdevice, bool& canSyncAll, bool& inSyncAll);

void showWindow(GLFWwindow*& window);

void processUpdateRequests();