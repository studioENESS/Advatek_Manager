cmake_minimum_required(VERSION 3.10)
# set the project name
project(AdvatekManager)

set(IMGUI_ROOT "${PROJECT_SOURCE_DIR}/External/imgui")
set(IMGUI_BACKENDS "${PROJECT_SOURCE_DIR}/External/imgui/backends")
set(PFD_ROOT "${PROJECT_SOURCE_DIR}/External/portable-file-dialogs")
set(LIBADV_ROOT "${PROJECT_SOURCE_DIR}/External/libAdvatek")

add_executable(AdvatekManager main.cpp
	gui_elements.cpp
	${IMGUI_ROOT}/imgui.cpp
	${IMGUI_ROOT}/imgui_draw.cpp
	${IMGUI_ROOT}/imgui_widgets.cpp
	${IMGUI_ROOT}/imgui_tables.cpp
	${IMGUI_ROOT}/backends/imgui_impl_opengl3.cpp
	${IMGUI_ROOT}/backends/imgui_impl_glfw.cpp
	${IMGUI_ROOT}/misc/cpp/imgui_stdlib.cpp
	${LIBADV_ROOT}/source/defines.cpp
	${LIBADV_ROOT}/source/udpclient.cpp
	${LIBADV_ROOT}/source/advatek_manager.cpp
	${LIBADV_ROOT}/source/SAdvatekDevice.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${IMGUI_ROOT} ${IMGUI_BACKENDS} ${PFD_ROOT} ${LIBADV_ROOT} ${LIBADV_ROOT}/include )

target_link_libraries(${PROJECT_NAME} GLESv2 EGL X11 pthread rt m c dl glfw)
