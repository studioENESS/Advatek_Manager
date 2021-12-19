# Advatek Assisitor

## Dependencies

  - [ImGui](https://github.com/ocornut/imgui) [Interavctive Manual](https://pthom.github.io/imgui_manual_online/manual/imgui_manual.html)  
  - [Boost](https://github.com/boostorg/boost)  

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

