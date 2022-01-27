// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#if defined(__ARM__)
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

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

advatek_manager adv;

int b_pollRequest = 0;
int b_refreshAdaptorsRequest = 0;
int id_networkConfigRequest = -1;
int currentAdaptor = 0;

int main(int, char**)
{

	refreshAdaptors();

	adv.poll();

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

	int window_w = 800;
	int window_h = 600;
    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(window_w, window_h, "Advatek Assistor", NULL, NULL);
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
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

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

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
		uint8_t buffer[100000];
		
		if (r_socket.available() > 0)
		{
			std::size_t bytes_transferred = r_socket.receive_from(boost::asio::buffer(buffer), receiver);
			if (bytes_transferred > 1) {  // we have data
				adv.process_udp_message(buffer);
			}
		}

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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

			ImGui::PushItemWidth(120);

			if (ImGui::BeginCombo("###Adaptor", adaptor_string.c_str(), 0))
			{
				for (int n = 1; n < networkAdaptors.size(); n++)
				{
					const bool is_selected = (currentAdaptor == n);
					if (ImGui::Selectable(networkAdaptors[n].c_str(), is_selected))
					{
						currentAdaptor = n;
						adaptor_string = networkAdaptors[n];
						if(r_socket.is_open()) {
							r_socket.close();	
						}
						//r_socket.open(boost::asio::ip::udp::v4());
						//r_socket.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(adaptor_string), AdvPort));

						receiver = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(adaptor_string), AdvPort);
						//receiver = boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), AdvPort);
						r_socket = boost::asio::ip::udp::socket(io_context, receiver);

						b_pollRequest = true;
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
				ImGui::PopItemWidth();
			}


			if (ImGui::Button("Search"))
			{
				b_pollRequest = true;
			} ImGui::SameLine();

			ImGui::Text("%li Device(s) Connected", adv.devices.size());

			for (uint8_t i = 0; i < adv.devices.size(); i++) {
				std::stringstream Title;
				Title << adv.devices[i]->Model << "	" << adv.devices[i]->Firmware << "	" << ipString(adv.devices[i]->CurrentIP) << "		" << "Temp: " << (float)adv.devices[i]->Temperature*0.1 << "		" << adv.devices[i]->Nickname;
				Title << "###" << macString(adv.devices[i]->Mac);
				bool node_open = ImGui::TreeNodeEx(Title.str().c_str(), ImGuiSelectableFlags_SpanAllColumns | ImGuiTreeNodeFlags_OpenOnArrow);

				if (node_open)
				{
					ImGui::Columns(1);
					ImGui::BeginTabBar("MyTabBar", ImGuiTabBarFlags_None);
					if (ImGui::BeginTabItem("Network"))
					{
						ImGui::Text("Static IP Address:");

						ImGui::PushItemWidth(30);

						ImGui::InputScalar(".##CurrentIP0", ImGuiDataType_U8, &adv.devices[i]->StaticIP[0], 0, 0, 0);

						ImGui::SameLine();
						ImGui::InputScalar(".##CurrentIP1", ImGuiDataType_U8, &adv.devices[i]->StaticIP[1], 0, 0, 0);

						ImGui::SameLine();
						ImGui::InputScalar(".##CurrentIP2", ImGuiDataType_U8, &adv.devices[i]->StaticIP[2], 0, 0, 0);

						ImGui::SameLine();
						ImGui::InputScalar("##CurrentIP3", ImGuiDataType_U8, &adv.devices[i]->StaticIP[3], 0, 0, 0);


						ImGui::Text("Static Subnet Mask:");

						ImGui::InputScalar(".##CurrentSM0", ImGuiDataType_U8, &adv.devices[i]->StaticSM[0], 0, 0, 0);

						ImGui::SameLine();
						ImGui::InputScalar(".##CurrentSM1", ImGuiDataType_U8, &adv.devices[i]->StaticSM[1], 0, 0, 0);

						ImGui::SameLine();
						ImGui::InputScalar(".##CurrentSM2", ImGuiDataType_U8, &adv.devices[i]->StaticSM[2], 0, 0, 0);

						ImGui::SameLine();
						ImGui::InputScalar("##CurrentSM3", ImGuiDataType_U8, &adv.devices[i]->StaticSM[3], 0, 0, 0);

						ImGui::PopItemWidth();

						ImGui::Text("IP Type: "); ImGui::SameLine();
						int tempDHCP = (int)adv.devices[i]->DHCP;
						if (ImGui::RadioButton("DHCP", &tempDHCP, 1)) {
							adv.devices[i]->DHCP = 1;
						} ImGui::SameLine();
						if (ImGui::RadioButton("Static", &tempDHCP, 0)) {
							adv.devices[i]->DHCP = 0;
						}

						if (ImGui::Button("Update Network Config"))
						{
							id_networkConfigRequest = i;
						}

						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Control"))
					{
						ImGui::Text("Ethernet Control: "); ImGui::SameLine();
						int tempProtocol = (int)adv.devices[i]->Protocol;
						if (ImGui::RadioButton("ArtNet", &tempProtocol, 1)) {
							adv.devices[i]->Protocol = 1;
						} ImGui::SameLine();
						if (ImGui::RadioButton("sACN (E1.31)", &tempProtocol, 0)) {
							adv.devices[i]->Protocol = 0;
						}
						ImGui::SameLine();
						static bool tempHoldLastFrame = (bool)adv.devices[i]->HoldLastFrame;
						if (ImGui::Checkbox("Hold LastFrame", &tempHoldLastFrame)) {
							adv.devices[i]->HoldLastFrame = tempHoldLastFrame;
						}

						bool * tempReversed = new bool[adv.devices[i]->NumOutputs];
						int  * tempEndUniverse = new int[adv.devices[i]->NumOutputs];
						int  * tempEndChannel = new int[adv.devices[i]->NumOutputs];

						ImGui::PushItemWidth(50);

						if (ImGui::BeginTable("table1", 11))
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

							for (int output = 0; output < adv.devices[i]->NumOutputs*0.5; output++)
							{
								ImGui::TableNextRow();

								setEndUniverseChannel((int)adv.devices[i]->OutputUniv[output], (int)adv.devices[i]->OutputChan[output], (int)adv.devices[i]->OutputPixels[output], (int)adv.devices[i]->OutputGrouping[output], tempEndUniverse[output], tempEndChannel[output]);
								ImGui::PushID(output);
								
								ImGui::TableNextColumn();
								ImGui::Text("Output %i", output + 1); ImGui::TableNextColumn();
								ImGui::InputScalar("##StartUniv", ImGuiDataType_U16, &adv.devices[i]->OutputUniv[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##StartChan", ImGuiDataType_U16, &adv.devices[i]->OutputChan[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##EndUniv", ImGuiDataType_U16, &tempEndUniverse[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##EndChan", ImGuiDataType_U16, &tempEndChannel[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##NumPix", ImGuiDataType_U16, &adv.devices[i]->OutputPixels[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##NullPix", ImGuiDataType_U8, &adv.devices[i]->OutputNull[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##ZigZag", ImGuiDataType_U16, &adv.devices[i]->OutputZig[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##Group", ImGuiDataType_U16, &adv.devices[i]->OutputGrouping[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("%##BrightLim", ImGuiDataType_U8, &adv.devices[i]->OutputBrightness[output], 0, 0, 0); ImGui::TableNextColumn();
								tempReversed[output] = (bool)adv.devices[i]->OutputReverse[output];
								if (ImGui::Checkbox("##Reversed", &tempReversed[output])) {
									adv.devices[i]->OutputReverse[output] = (uint8_t)tempReversed[output];
								}
								ImGui::PopID();
								
							}
							ImGui::EndTable();
						}


						ImGui::PopItemWidth();

						//ImGui::Text("DMX512 Outputs");
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("LEDs"))
					{
						ImGui::PushItemWidth(120);
						ImGui::Combo("Pixel IC", &adv.devices[i]->CurrentDriver, adv.devices[i]->DriverNames, adv.devices[i]->NumDrivers);
						ImGui::Combo("Clock Speed", &adv.devices[i]->CurrentDriverSpeed, DriverSpeedsMhz, 12);
						bool tempExpanded = (bool)adv.devices[i]->CurrentDriverExpanded;
						if (ImGui::Checkbox("Expanded Mode", &tempExpanded)) {
							adv.devices[i]->CurrentDriverExpanded = (uint8_t)tempExpanded;
						}


						ImGui::Text("Set All  "); ImGui::SameLine();
						int tempAllColOrder = adv.devices[i]->OutputColOrder[0];
						if (ImGui::Combo("Order ##all", &tempAllColOrder, adv.RGBW_Order, 24)) {
							for (uint8_t output = 0; output < adv.devices[i]->NumOutputs*0.5; output++) {
								adv.devices[i]->OutputColOrder[output] = (uint8_t)tempAllColOrder;
							}
						}

						//int * tempOutputColOrder = new int[adv.devices[i]->NumOutputs];
						//for (uint8_t output = 0; output < adv.devices[i]->NumOutputs*0.5; output++) {
						//	ImGui::PushID(output);
						//	ImGui::Text("Output %02i", output + 1); ImGui::SameLine();
						//	tempOutputColOrder[output] = adv.devices[i]->OutputColOrder[output];
						//	if (ImGui::Combo("Order", &tempOutputColOrder[output], adv.RGBW_Order, 24)) {
						//		adv.devices[i]->OutputColOrder[output] = (uint8_t)tempOutputColOrder[output];
						//	}
						//	ImGui::PopID();
						//}

						ImGui::Separator();
						ImGui::Text("Gamma Correction only applied to chips that are higher then 8bit:");

						if (ImGui::SliderFloat("Red", &adv.devices[i]->Gammaf[0], 1.0, 3.0, "%.01f")) {
							adv.devices[i]->Gamma[0] = (int)(adv.devices[i]->Gammaf[0] * 10);
						};
						if (ImGui::SliderFloat("Green", &adv.devices[i]->Gammaf[1], 1.0, 3.0, "%.01f")) {
							adv.devices[i]->Gamma[1] = (int)(adv.devices[i]->Gammaf[1] * 10);
						};
						if (ImGui::SliderFloat("Blue", &adv.devices[i]->Gammaf[2], 1.0, 3.0, "%.01f")) {
							adv.devices[i]->Gamma[2] = (int)(adv.devices[i]->Gammaf[2] * 10);
						};
						if (ImGui::SliderFloat("White", &adv.devices[i]->Gammaf[3], 1.0, 3.0, "%.01f")) {
							adv.devices[i]->Gamma[3] = (int)(adv.devices[i]->Gammaf[3] * 10);
						};
						ImGui::PopItemWidth();
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Test"))
					{
						ImGui::Text("Prior to running test mode all other setings should be saved to controller (Press Update).");

						ImGui::PushItemWidth(200);

						if (ImGui::Combo("Set Test", &adv.devices[i]->TestMode, TestModes, 9)) {
							adv.setTest(i);
						}

						if ((adv.devices[i]->TestMode == 6) || (adv.devices[i]->TestMode == 8)) {
							float tempTestCols[4];
							tempTestCols[0] = ((float)adv.devices[i]->TestCols[0] / 255);
							tempTestCols[1] = ((float)adv.devices[i]->TestCols[1] / 255);
							tempTestCols[2] = ((float)adv.devices[i]->TestCols[2] / 255);
							tempTestCols[3] = ((float)adv.devices[i]->TestCols[3] / 255);

							if (ImGui::ColorEdit4("Test Colour", tempTestCols)) {
								adv.devices[i]->TestCols[0] = (int)(tempTestCols[0] * 255);
								adv.devices[i]->TestCols[1] = (int)(tempTestCols[1] * 255);
								adv.devices[i]->TestCols[2] = (int)(tempTestCols[2] * 255);
								adv.devices[i]->TestCols[3] = (int)(tempTestCols[3] * 255);
							}

							if (ImGui::Checkbox("Cycle Colour Per Output", &adv.devices[i]->TestModeCycle)) {
								for (uint8_t output = 0; output < adv.devices[i]->NumOutputs*0.5; output++) {
									ImGui::PushID(output);
									ImGui::Text("Output %02i", output + 1); ImGui::SameLine();
									
									ImGui::PopID();
								}
							}
							else {
								adv.setTest(i);
							}
						}

						ImGui::PopItemWidth();
						ImGui::PushItemWidth(150);
						ImGui::Text("All Outputs (0)"); ImGui::SameLine();
						if (ImGui::SliderInt("Output Channel", (int*)&adv.devices[i]->TestOutputNum, 0, (int)(adv.devices[i]->NumOutputs*0.5))) {
							adv.setTest(i);
						}
						ImGui::PopItemWidth();

						ImGui::EndTabItem();
					}


					if (ImGui::BeginTabItem("Misc"))
					{
						ImGui::PushItemWidth(200);
						char sName[40];
						memcpy(sName, adv.devices[i]->Nickname, 40);
						if (ImGui::InputText("Nickname", sName, 40)) {
							memset(adv.devices[i]->Nickname, 0, 40);
							strcpy(adv.devices[i]->Nickname, sName);
						}
						ImGui::PopItemWidth();

						ImGui::PushItemWidth(30);

						ImGui::InputScalar("Fan Control On Temp", ImGuiDataType_U8, &adv.devices[i]->MaxTargetTemp, 0, 0, 0);
						ImGui::PopItemWidth();

						int * tempVoltage = new int[adv.devices[i]->NumBanks];

						for (uint8_t bank = 0; bank < adv.devices[i]->NumBanks; bank++) {
							ImGui::PushID(bank);
							ImGui::Text("Bank %i: %.2f V", bank + 1, ((float)adv.devices[i]->VoltageBanks[bank] / 10.f));
							ImGui::PopID();
						}

						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();

					ImGui::Separator();

					if (ImGui::Button("Update"))
					{
						adv.updateDevice(i);
						// b_pollRequest = true; 
					}

					ImGui::TreePop();
				}
			}

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
			refreshAdaptors();
			b_refreshAdaptorsRequest = false;
		}
		if (b_pollRequest) {
			adv.poll();
			b_pollRequest = false;
		}
		if (id_networkConfigRequest > -1) {
			adv.bc_networkConfig(id_networkConfigRequest);
			id_networkConfigRequest = -1;
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
