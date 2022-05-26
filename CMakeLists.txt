cmake_minimum_required(VERSION 3.10)
# set the project name
project(advatek_assistor)

set(BOOST_ROOT "${PROJECT_SOURCE_DIR}/External/boost")
set(Boost_INCLUDE_DIR  "${PROJECT_SOURCE_DIR}/External/boost")
set(Boost_LIBRARY_DIR "${PROJECT_SOURCE_DIR}/External/boost/libs")
#find_package(Boost REQUIRED Components asio) # add the executable

set(IMGUI_ROOT "${PROJECT_SOURCE_DIR}/External/imgui")
set(IMGUI_BACKENDS "${PROJECT_SOURCE_DIR}/External/imgui/backends")

set(PFD_ROOT "${PROJECT_SOURCE_DIR}/External/portable-file-dialogs")

add_executable(advatek_assistor main.cpp
	defines.cpp
	advatek_assistor.cpp
	udpclient.cpp
	gui_elements.cpp
	${IMGUI_ROOT}/imgui.cpp
	${IMGUI_ROOT}/imgui_draw.cpp
	${IMGUI_ROOT}/imgui_widgets.cpp
	${IMGUI_ROOT}/imgui_tables.cpp
	${IMGUI_ROOT}/backends/imgui_impl_opengl3.cpp
	${IMGUI_ROOT}/backends/imgui_impl_glfw.cpp
	${IMGUI_ROOT}/misc/cpp/imgui_stdlib.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${BOOST_ROOT} ${IMGUI_ROOT} ${IMGUI_BACKENDS} ${PFD_ROOT} )

target_link_libraries(${PROJECT_NAME} GLESv2 EGL X11 pthread rt m c dl glfw)
