# Advatek Assisitor

![preview](img/preview2.png)

## Dependencies

  - [ImGui](https://github.com/ocornut/imgui) [Interavctive Manual](https://pthom.github.io/imgui_manual_online/manual/imgui_manual.html)  
  - [Boost](https://github.com/boostorg/boost)  
  - [Portable File Dialogs](https://github.com/samhocevar/portable-file-dialogs)  

## Build
    
    git clone <this repo>
    git submodule update --init
    
    cd External/boost
    git submodule update --init

    ./bootstrap.sh or ./bootstrap.bat
    ./b2 or b2
    
    sudo apt-get install libglfw3-dev
    
    mkdir build && cd build
    
    cmake ../ 
    
    make


 **For debug use GDB**

    cmake -DCMAKE_BUILD_TYPE=Debug ../
    
    make
    
    gdb ./advatek_assistor
`
> To use the WiFi for internet make sure you set the Wired connection to `Only use connection for resources on it's network`

