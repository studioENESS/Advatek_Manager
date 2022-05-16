#include "gui_elements.h"

#define Version "1.3.0"

#ifndef DEBUG
#pragma comment(linker, "/SUBSYSTEM:Windows /ENTRY:mainCRTStartup")
#endif

int main(int, char**)
{	
	GLFWwindow* window;
	int window_w, window_h;
	float scale;

    setupWindow(window, window_w, window_h, scale);

	// Init Advatek Manager
	applog.AddLog("[INFO] Advatek Assistor v%s\n", Version);
	adv.refreshAdaptors();
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
		glfwPollEvents();
		glfwGetWindowSize(window, &window_w, &window_h);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
		currTime = ImGui::GetTime();
		adv.listen();

		if (currTime - lastSoftPoll > rePollTime) {
			adv.softPoll();
			lastSoftPoll = currTime;
			//applog.AddLog(("[INFO] Polling using network adaptor " + adaptor_string + " ...\n").c_str());
		}

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(window_w, window_h));
		showWindow(window, scale);
		processUpdateRequests();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
