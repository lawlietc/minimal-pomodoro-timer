# 极简番茄钟应用 / Minimal Pomodoro Timer

这是一个极简的Windows番茄钟应用，内存占用不到2MB。

## 文件说明

- `pomodoro_simple.c` - 主程序源代码
- `pomodoro_final.rc` - 资源文件（包含图标和版本信息）
- `pomodoro_final.res` - 编译后的资源文件
- `pomodoro_settings.ini` - 配置文件（存储用户设置）
- `resource\alarm.ico` - 应用程序图标文件
- `release\Little Pomodoro.exe` - 最终编译的可执行文件

## 编译说明

要编译这个项目，你需要安装GCC编译器（如MinGW）并执行以下命令：

```bash
windres pomodoro_final.rc -O coff -o pomodoro_final.res
gcc -mwindows -O2 -s -D_UNICODE -DUNICODE -o "Little Pomodoro.exe" pomodoro_simple.c pomodoro_final.res -luser32 -lshell32 -lkernel32 -ladvapi32
```

## 核心功能

- 极简
- 内存占用不到2MB
- 直接使用Windows系统的通知做提醒

## 操作说明

- 左键点击窗口 ：开始/暂停计时
- 右键点击窗口 ：隐藏主窗口
- 点击蓝色"设置"文字 ：进入设置页面，可自行设置工作时间和休息时间，设置将自动保存在同目录 `pomodoro_settings.ini` 文件中
- Shift+左键拖动 ：移动窗口位置
- 双击托盘图标 ：显示/隐藏主窗口
- 右键托盘图标 ：显示菜单

---

# Minimal Pomodoro Timer

This folder contains all the source code and resource files for the final version of the Pomodoro Timer application.

## File Descriptions

- `pomodoro_simple.c` - Main program source code
- `pomodoro_final.rc` - Resource file (includes icon and version information)
- `pomodoro_final.res` - Compiled resource file
- `pomodoro_settings.ini` - Configuration file (stores user settings)
- `resource\alarm.ico` - Application icon file
- `release\Little Pomodoro.exe` - Final compiled executable file

## Compilation Instructions

To compile this project, you need to install a GCC compiler (such as MinGW) and execute the following commands:

```bash
windres pomodoro_final.rc -O coff -o pomodoro_final.res
gcc -mwindows -O2 -s -D_UNICODE -DUNICODE -o "Little Pomodoro.exe" pomodoro_simple.c pomodoro_final.res -luser32 -lshell32 -lkernel32 -ladvapi32
```

## Core Features

- Minimalist
- Memory usage under 2MB
- Uses Windows system notifications for reminders

## Operation Instructions

- Left-click on window: Start/Pause timer
- Right-click on window: Hide main window
- Click blue "Settings" text: Enter settings page to set work and break times, settings will be automatically saved in `pomodoro_settings.ini` file in the same directory
- Shift+Left-drag: Move window position
- Double-click tray icon: Show/Hide main window
- Right-click tray icon: Show menu