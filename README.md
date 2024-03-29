# PPFC

A simple fc emulator implemented in c++11 using SDL2 (for Windowns and Linux)

![](balloon.gif) ![](mario.gif) ![](F-1-Race.gif) 

# Info

- 目前仅支持Mapper000，但并不保证其所有游戏能玩
- 支持自适应窗口
- 支持Linux平台，不过仅在Ubuntu18上测试过

# Environment

- SDL2-2.0.12
- vs2017
- Windowns 10

# Make

- Windowns: 

  - vs2017：用`Release x86`生成解决方案，然后将lib/SDL2-2.0.12/lib/x86/SDL2.dll 复制到ppfc.exe 所在目录

  - mingw64：

    ```
    cd ppfc
    g++ -o ppfc.exe src/*.cpp -std=c++11 -O1 "-Ilib\x86_64-w64-mingw32\include\SDL2" "-Llib\x86_64-w64-mingw32\lib" -lSDL2 -lSDL2main
    ```
    然后将lib/x86_64-w64-mingw32/bin/SDL2.dll 复制到ppfc.exe 所在目录
    

- Linux: 需要另外安装SDL2

  ```
  cd ppfc
  g++ -o ppfc src/*.cpp -std=c++11 -O1 -lpthread $(sdl2-config --cflags --libs)
  ```

# Usage

- ppfc.exe \<nes rom path\>
- 用鼠标将ROM拖曳到ppfc.exe

# Controller

**Player 1**

| Button | Key  |
| :----: | :--: |
|   Up   |  W   |
|  Down  |  S   |
|  Left  |  A   |
| Right  |  D   |
|   A    |  J   |
|   B    |  K   |
| Select |  U   |
| Start  |  I   |

**Player 2**

| Button |   Key    |
| :----: | :------: |
|   Up   |    ↑     |
|  Down  |    ↓     |
|  Left  |    ←     |
| Right  |    →     |
|   A    | Keypad 2 |
|   B    | Keypad 3 |
| Select | Keypad 5 |
| Start  | Keypad 6 |

**Other**

| Action |   Key    |
| :----: | :------: |
| Reset  | Ctrl + 3 |

# TODO

- [ ] Save/Load
- [ ] APU
- [ ] Trainer (金手指)
- [ ] 将该项目移植到stm32或esp32上运行

# Reference

- [StepFC](https://github.com/dustpg/StepFC)
- http://obelisk.me.uk/6502/
- [Nesdev Wiki](http://wiki.nesdev.com/w/index.php/Nesdev_Wiki)