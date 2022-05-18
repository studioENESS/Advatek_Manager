#include "gui_elements.h"

#define Version "1.3.1"

#ifndef DEBUG
#pragma comment(linker, "/SUBSYSTEM:Windows /ENTRY:mainCRTStartup")
#endif

int main(int, char**)
{	
	GLFWwindow* window;

    setupWindow(window);

	// Init Advatek Manager
	applog.AddLog("[INFO] Advatek Assistor v%s\n", Version);
	adv.refreshAdaptors();
	if (adv.networkAdaptors.size() > 0) {
		adaptor_string = adv.networkAdaptors[0];
		adv.poll();
		applog.AddLog(("[INFO] Polling using network adaptor " + adaptor_string + "...\n").c_str());
		s_loopVar.lastPoll = s_loopVar.currTime;
	}

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
		glfwPollEvents();
		glfwGetWindowSize(window, &s_loopVar.window_w, &s_loopVar.window_h);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
	
		s_loopVar.currTime = ImGui::GetTime();

		adv.listen();

		if (s_loopVar.currTime - s_loopVar.lastPoll > s_loopVar.rePollTime) {
			adv.softPoll();
			s_loopVar.lastPoll = s_loopVar.currTime;
			//applog.AddLog(("[INFO] Polling using network adaptor " + adaptor_string + "...\n").c_str());
		}

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(s_loopVar.window_w, s_loopVar.window_h));
		showWindow(window);
		processUpdateRequests();
		adv.sortAllDevices();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
