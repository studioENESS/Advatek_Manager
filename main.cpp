#include "gui_elements.h"
#include "standard_json_config.h"

#define Version "1.3.0"

#ifndef DEBUG
#pragma comment(linker, "/SUBSYSTEM:Windows /ENTRY:mainCRTStartup")
#endif

int main(int, char**)
{	
	GLFWwindow* window;
	float scale;
	int window_w, window_h, center_y, center_x;

    setupWindow(window_w, window_h, scale, center_x, center_y, window);

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
		currTime = ImGui::GetTime();
		adv.listen();

        glfwPollEvents();

		glfwGetWindowSize(window, &window_w, &window_h);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

		if (currTime - lastSoftPoll > rePollTime) {
			adv.softPoll();
			lastSoftPoll = currTime;
			//applog.AddLog(("[INFO] Polling using network adaptor " + adaptor_string + " ...\n").c_str());
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

			showResult(result);

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

					ImGui::Spacing();

					showDevices(adv.connectedDevices, true);

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

						for(const auto& jsonData:JSONControllers)
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
							} else {
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
					
					showDevices(adv.virtualDevices, false);

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			ImGui::Spacing();
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
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

		if (b_refreshAdaptorsRequest) {
			b_refreshAdaptorsRequest = false;
			adv.refreshAdaptors();
			if (adv.networkAdaptors.size() > 0) {
				adaptor_string = adv.networkAdaptors[0];
				adv.poll();
				applog.AddLog(("[INFO] Polling using network adaptor " + adaptor_string + " ...\n").c_str());
				lastPoll = currTime;
			} else {
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

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
