#include <windows.h>
#include <shellapi.h>
#include <stdio.h>

// 计时器状态
typedef struct {
    int workDuration;      // 工作时长(秒)
    int breakDuration;     // 休息时长(秒)
    int remainingTime;     // 剩余时间(秒)
    BOOL isWorking;        // 是否工作中
    BOOL isPaused;         // 是否暂停
    BOOL isRunning;        // 是否运行中
} TimerState;

// 应用程序数据
typedef struct {
    HWND hWnd;              // 主窗口句柄
    HWND hTimeLabel;       // 时间标签
    HWND hStatusLabel;     // 状态标签
    HWND hSettingsButton;  // 设置按钮
    HWND hWorkEdit;        // 工作时长输入框
    HWND hBreakEdit;       // 休息时长输入框
    HWND hSaveButton;      // 保存按钮
    HWND hCancelButton;    // 取消按钮
    HWND hWorkLabel;       // 工作时长标签
    HWND hBreakLabel;      // 休息时长标签
    HMENU hTrayMenu;      // 托盘菜单
    NOTIFYICONDATAW nid;   // 托盘图标数据
    TimerState timer;      // 计时器状态
    UINT_PTR timerId;     // 计时器ID
    BOOL isSettingsMode;   // 是否处于设置模式
    BOOL isSettingsButtonHovered;  // 设置按钮悬停状态
    int tempWorkMinutes;   // 临时工作时长（分钟）
    int tempBreakMinutes;  // 临时休息时长（分钟）
} AppData;

// 全局变量
static AppData g_app = {0};
static const wchar_t* g_className = L"PomodoroTimerClass";

// 消息定义
#define WM_TRAYICON (WM_USER + 1)
#define IDI_MAIN_ICON 101
#define ID_TRAY_ICON 1001
#define ID_TRAY_EXIT 1002
#define ID_TRAY_START 1003
#define ID_TRAY_RESET 1004
#define ID_TIMER 2001
#define ID_SETTINGS_BUTTON 3001
#define ID_WORK_EDIT 3002
#define ID_BREAK_EDIT 3003
#define ID_SAVE_BUTTON 3004
#define ID_CANCEL_BUTTON 3005

// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateMainWindow(HINSTANCE hInstance);
void CreateTrayIcon(HWND hWnd);
void RemoveTrayIcon(HWND hWnd);
void ShowTrayMenu(HWND hWnd);
void UpdateTimerDisplay();
void SwitchTimerMode();
void StartTimer();
void PauseTimer();
void ResetTimer();
void SaveSettingsToINI();
void LoadSettingsFromINI();
void ShowNotification(const wchar_t* title, const wchar_t* message);
void ShowMainView();
void ShowSettingsView();
void SaveSettings();



// 格式化时间字符串
void FormatTime(int seconds, wchar_t* buffer, int bufferSize) {
    int minutes = seconds / 60;
    int secs = seconds % 60;
    swprintf_s(buffer, bufferSize, L"%02d:%02d", minutes, secs);
}

// 窗口过程
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // 初始化计时器状态
            g_app.timer.workDuration = 27 * 60;  // 27分钟
            g_app.timer.breakDuration = 3 * 60;  // 3分钟
            g_app.timer.remainingTime = g_app.timer.workDuration;
            g_app.timer.isWorking = TRUE;
            g_app.timer.isPaused = TRUE;
            g_app.timer.isRunning = FALSE;
            g_app.isSettingsMode = FALSE;
    g_app.isSettingsButtonHovered = FALSE;  // 初始化悬停状态
    g_app.tempWorkMinutes = 27;
    g_app.tempBreakMinutes = 3;
            
            // 加载保存的设置
            LoadSettingsFromINI();
            
            // 创建时间标签 - 使用新的深色配色
            g_app.hTimeLabel = CreateWindowW(
                L"STATIC", L"00:00",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                10, 35, 180, 50,  // 调整位置和大小
                hwnd, NULL, GetModuleHandle(NULL), NULL
            );
            
            // 更新显示为正确的INI配置时间
            UpdateTimerDisplay();
            
            // 设置大字体 - 使用更现代的字体和颜色
            HFONT hFont = CreateFontW(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            SendMessageW(g_app.hTimeLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            // 创建状态标签 - 使用新的深色配色
            g_app.hStatusLabel = CreateWindowW(
                L"STATIC", L"工作中 - 点击开始",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                10, 90, 180, 20,  // 调整位置
                hwnd, NULL, GetModuleHandle(NULL), NULL
            );
            
            // 创建提示标签
            HWND hHintLabel = CreateWindowW(
                L"STATIC", L"点击窗口开始计时",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                10, 115, 180, 15,  // 新增提示标签
                hwnd, NULL, GetModuleHandle(NULL), NULL
            );
            
            // 设置提示标签的小字体
            HFONT hSmallFont = CreateFontW(10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            SendMessageW(hHintLabel, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
            
            // 创建设置按钮（文字按钮样式）
            g_app.hSettingsButton = CreateWindowW(
                L"STATIC", L"设置",  // 使用STATIC控件实现文字按钮
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY,
                150, 10, 40, 20,  // 右上角位置，稍小一些
                hwnd, (HMENU)ID_SETTINGS_BUTTON, GetModuleHandle(NULL), NULL
            );
            
            // 设置按钮字体（使用稍小的字体）
            HFONT hLinkFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, TRUE, 0, DEFAULT_CHARSET, 
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            SendMessageW(g_app.hSettingsButton, WM_SETFONT, (WPARAM)hLinkFont, TRUE);
            
            // 创建托盘图标
            CreateTrayIcon(hwnd);
            
            // 设置窗口位置 - 考虑标题栏高度
            RECT rect;
            SystemParametersInfoW(SPI_GETWORKAREA, 0, &rect, 0);
            SetWindowPos(hwnd, HWND_TOPMOST, 
                rect.right - 230, rect.bottom - 220,  // 调整位置
                220, 200, SWP_SHOWWINDOW);  // 使用新的窗口尺寸
            
            return 0;
        }
        
        case WM_TIMER: {
            if (wParam == ID_TIMER && g_app.timer.isRunning && !g_app.timer.isPaused) {
                if (g_app.timer.remainingTime > 0) {
                    g_app.timer.remainingTime--;
                    UpdateTimerDisplay();
                } else {
                    // 时间到，切换模式
                    SwitchTimerMode();
                }
            }
            return 0;
        }
        
        case WM_TRAYICON: {
            if (lParam == WM_RBUTTONUP) {
                ShowTrayMenu(hwnd);
            } else if (lParam == WM_LBUTTONDBLCLK) {
                ShowWindow(hwnd, IsWindowVisible(hwnd) ? SW_HIDE : SW_SHOW);
            }
            return 0;
        }
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case ID_TRAY_START:
                    if (g_app.timer.isRunning && !g_app.timer.isPaused) {
                        PauseTimer();
                    } else {
                        StartTimer();
                    }
                    break;
                case ID_TRAY_RESET:
                    ResetTimer();
                    break;
                case ID_TRAY_EXIT:
                    PostQuitMessage(0);
                    break;
                case ID_SETTINGS_BUTTON:
                    if (!g_app.isSettingsMode) {
                        ShowSettingsView();
                    }
                    break;
                case ID_SAVE_BUTTON:
                    SaveSettings();
                    break;
                case ID_CANCEL_BUTTON:
                    ShowMainView();
                    break;
            }
            return 0;
        }
        
        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            HWND hControl = (HWND)lParam;
            
            // 设置按钮特殊处理
            if (hControl == g_app.hSettingsButton) {
                if (g_app.isSettingsButtonHovered) {
                    SetTextColor(hdcStatic, RGB(150, 220, 255)); // 悬停时更亮的蓝色
                } else {
                    SetTextColor(hdcStatic, RGB(100, 200, 255)); // 正常状态的蓝色
                }
                SetBkColor(hdcStatic, RGB(45, 45, 45)); // 深灰色背景
                return (INT_PTR)CreateSolidBrush(RGB(45, 45, 45));
            }
            
            // 其他静态控件使用默认配色
            SetTextColor(hdcStatic, RGB(224, 224, 224)); // 浅灰色文字
            SetBkColor(hdcStatic, RGB(45, 45, 45)); // 深灰色背景
            return (INT_PTR)CreateSolidBrush(RGB(45, 45, 45)); // 返回深灰色画刷
        }
        
        case WM_LBUTTONDOWN: {
            // 按住Shift键点击窗口可以拖动窗口
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                ReleaseCapture();
                SendMessage(hwnd, WM_SYSCOMMAND, 0xF012, 0); // SC_DRAGMOVE
                return 0;
            }
            
            // 点击窗口开始/暂停计时（仅在主界面有效）
            if (!g_app.isSettingsMode) {
                if (g_app.timer.isRunning && !g_app.timer.isPaused) {
                    PauseTimer();
                } else {
                    StartTimer();
                }
            }
            return 0;
        }
        
        case WM_RBUTTONDOWN: {
            // 右键点击窗口隐藏主窗口，效果与双击托盘图标一致
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            // 检测鼠标是否在设置按钮上
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            
            RECT rect;
            GetWindowRect(g_app.hSettingsButton, &rect);
            ScreenToClient(hwnd, (LPPOINT)&rect.left);
            ScreenToClient(hwnd, (LPPOINT)&rect.right);
            
            BOOL wasHovered = g_app.isSettingsButtonHovered;
            g_app.isSettingsButtonHovered = PtInRect(&rect, pt);
            
            if (g_app.isSettingsButtonHovered) {
                SetCursor(LoadCursorW(NULL, IDC_HAND)); // 手型光标
            }
            
            // 如果悬停状态改变，重绘按钮
            if (wasHovered != g_app.isSettingsButtonHovered) {
                InvalidateRect(g_app.hSettingsButton, NULL, TRUE);
            }
            return 0;
        }
        
        case WM_LBUTTONUP: {
            // 检测鼠标是否在设置按钮上释放
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            
            RECT rect;
            GetWindowRect(g_app.hSettingsButton, &rect);
            ScreenToClient(hwnd, (LPPOINT)&rect.left);
            ScreenToClient(hwnd, (LPPOINT)&rect.right);
            
            if (PtInRect(&rect, pt) && !g_app.isSettingsMode) {
                ShowSettingsView();
            }
            return 0;
        }
        
        case WM_SYSCOMMAND: {
            if (wParam == SC_MINIMIZE) {
                // 最小化时隐藏到托盘，而不是关闭
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            }
            break;
        }
        
        case WM_DESTROY:
            RemoveTrayIcon(hwnd);
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 创建主窗口
void CreateMainWindow(HINSTANCE hInstance) {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    // 使用应用程序资源图标
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_MAIN_ICON));
    if (!wc.hIcon) {
        wc.hIcon = LoadIconW(NULL, IDI_APPLICATION); // 如果加载失败，使用默认图标
    }
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(45, 45, 45)); // 深灰色背景
    wc.lpszMenuName = NULL;
    wc.lpszClassName = g_className;
    wc.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_MAIN_ICON));
    if (!wc.hIconSm) {
        wc.hIconSm = LoadIconW(NULL, IDI_APPLICATION); // 如果加载失败，使用默认图标
    }
    
    RegisterClassExW(&wc);
    
    g_app.hWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        g_className, L"番茄钟",
        WS_POPUP | WS_VISIBLE, // 去掉标题栏，使用无边框窗口
        CW_USEDEFAULT, CW_USEDEFAULT, 220, 200,  // 窗口尺寸
        NULL, NULL, hInstance, NULL
    );
}

// 创建托盘图标
void CreateTrayIcon(HWND hWnd) {
    g_app.nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_app.nid.hWnd = hWnd;
    g_app.nid.uID = ID_TRAY_ICON;
    g_app.nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_app.nid.uCallbackMessage = WM_TRAYICON;
    // 使用与应用程序相同的图标资源
    g_app.nid.hIcon = LoadIconW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDI_MAIN_ICON));
    if (!g_app.nid.hIcon) {
        g_app.nid.hIcon = LoadIconW(NULL, IDI_APPLICATION); // 如果加载失败，使用默认图标
    }
    wcsncpy(g_app.nid.szTip, L"番茄钟 - 工作中", sizeof(g_app.nid.szTip)/sizeof(wchar_t) - 1);
    g_app.nid.szTip[sizeof(g_app.nid.szTip)/sizeof(wchar_t) - 1] = L'\0';
    
    Shell_NotifyIconW(NIM_ADD, &g_app.nid);
}

// 移除托盘图标
void RemoveTrayIcon(HWND hWnd) {
    Shell_NotifyIconW(NIM_DELETE, &g_app.nid);
}

// 显示托盘菜单
void ShowTrayMenu(HWND hWnd) {
    HMENU hMenu = CreatePopupMenu();
    
    if (g_app.timer.isRunning && !g_app.timer.isPaused) {
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_START, L"暂停");
    } else {
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_START, L"开始");
    }
    
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_RESET, L"重置");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"退出");
    
    POINT pt;
    GetCursorPos(&pt);
    
    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}

// 更新计时器显示
void UpdateTimerDisplay() {
    wchar_t timeStr[32];
    FormatTime(g_app.timer.remainingTime, timeStr, 32);
    SetWindowTextW(g_app.hTimeLabel, timeStr);
    
    const wchar_t* statusText = g_app.timer.isWorking ? 
        (g_app.timer.isPaused ? L"工作中 - 点击开始" : L"工作中") :
        (g_app.timer.isPaused ? L"休息中 - 点击开始" : L"休息中");
    SetWindowTextW(g_app.hStatusLabel, statusText);
    
    // 更新托盘提示
    const wchar_t* trayTip = g_app.timer.isWorking ? L"番茄钟 - 工作中" : L"番茄钟 - 休息中";
    wcsncpy(g_app.nid.szTip, trayTip, sizeof(g_app.nid.szTip)/sizeof(wchar_t) - 1);
    g_app.nid.szTip[sizeof(g_app.nid.szTip)/sizeof(wchar_t) - 1] = L'\0';
    Shell_NotifyIconW(NIM_MODIFY, &g_app.nid);
}

// 切换计时器模式
void SwitchTimerMode() {
    g_app.timer.isWorking = !g_app.timer.isWorking;
    g_app.timer.remainingTime = g_app.timer.isWorking ? 
        g_app.timer.workDuration : g_app.timer.breakDuration;
    
    const wchar_t* title = L"番茄钟";
    const wchar_t* message = g_app.timer.isWorking ? L"休息结束，开始工作！" : L"开始休息！";
    ShowNotification(title, message);
    
    UpdateTimerDisplay();
}

// 开始计时器
void StartTimer() {
    g_app.timer.isRunning = TRUE;
    g_app.timer.isPaused = FALSE;
    SetTimer(g_app.hWnd, ID_TIMER, 1000, NULL);
    UpdateTimerDisplay();
}

// 暂停计时器
void PauseTimer() {
    g_app.timer.isPaused = TRUE;
    KillTimer(g_app.hWnd, ID_TIMER);
    UpdateTimerDisplay();
}

// 重置计时器
void ResetTimer() {
    g_app.timer.isRunning = FALSE;
    g_app.timer.isPaused = TRUE;
    g_app.timer.isWorking = TRUE;
    g_app.timer.remainingTime = g_app.timer.workDuration;
    KillTimer(g_app.hWnd, ID_TIMER);
    UpdateTimerDisplay();
}

// 显示系统通知
void ShowNotification(const wchar_t* title, const wchar_t* message) {
    // 使用Windows 10/11的通知API
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = g_app.hWnd;
    nid.uID = ID_TRAY_ICON;
    nid.uFlags = NIF_INFO | NIF_ICON;
    nid.dwInfoFlags = NIIF_USER;  // 使用NIIF_USER标志来使用自定义图标
    // 使用与应用程序相同的图标资源
    nid.hIcon = LoadIconW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDI_MAIN_ICON));
    if (!nid.hIcon) {
        nid.hIcon = LoadIconW(NULL, IDI_APPLICATION); // 如果加载失败，使用默认图标
    }
    wcsncpy(nid.szInfoTitle, title, sizeof(nid.szInfoTitle)/sizeof(wchar_t) - 1);
    wcsncpy(nid.szInfo, message, sizeof(nid.szInfo)/sizeof(wchar_t) - 1);
    nid.szInfoTitle[sizeof(nid.szInfoTitle)/sizeof(wchar_t) - 1] = L'\0';
    nid.szInfo[sizeof(nid.szInfo)/sizeof(wchar_t) - 1] = L'\0';
    
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

// 显示主界面
void ShowMainView() {
    g_app.isSettingsMode = FALSE;
    
    // 显示主界面控件
    ShowWindow(g_app.hTimeLabel, SW_SHOW);
    ShowWindow(g_app.hStatusLabel, SW_SHOW);
    ShowWindow(g_app.hSettingsButton, SW_SHOW);
    
    // 隐藏设置界面控件（包括标签）
    if (g_app.hWorkEdit) ShowWindow(g_app.hWorkEdit, SW_HIDE);
    if (g_app.hBreakEdit) ShowWindow(g_app.hBreakEdit, SW_HIDE);
    if (g_app.hSaveButton) ShowWindow(g_app.hSaveButton, SW_HIDE);
    if (g_app.hCancelButton) ShowWindow(g_app.hCancelButton, SW_HIDE);
    if (g_app.hWorkLabel) ShowWindow(g_app.hWorkLabel, SW_HIDE);
    if (g_app.hBreakLabel) ShowWindow(g_app.hBreakLabel, SW_HIDE);
}

// 显示设置界面
void ShowSettingsView() {
    g_app.isSettingsMode = TRUE;
    
    // 隐藏主界面控件
    ShowWindow(g_app.hTimeLabel, SW_HIDE);
    ShowWindow(g_app.hStatusLabel, SW_HIDE);
    ShowWindow(g_app.hSettingsButton, SW_HIDE);
    
    // 创建设置界面控件（如果不存在）
    if (!g_app.hWorkEdit) {
        // 工作时长标签
        g_app.hWorkLabel = CreateWindowW(
            L"STATIC", L"工作时长（分钟）:",
            WS_CHILD | WS_VISIBLE,
            10, 20, 180, 20,
            g_app.hWnd, NULL, GetModuleHandle(NULL), NULL
        );
        
        // 工作时长输入框
        wchar_t workBuffer[10];
        swprintf_s(workBuffer, 10, L"%d", g_app.tempWorkMinutes);
        g_app.hWorkEdit = CreateWindowW(
            L"EDIT", workBuffer,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
            10, 40, 180, 25,
            g_app.hWnd, (HMENU)ID_WORK_EDIT, GetModuleHandle(NULL), NULL
        );
        
        // 休息时长标签
        g_app.hBreakLabel = CreateWindowW(
            L"STATIC", L"休息时长（分钟）:",
            WS_CHILD | WS_VISIBLE,
            10, 70, 180, 20,
            g_app.hWnd, NULL, GetModuleHandle(NULL), NULL
        );
        
        // 休息时长输入框
        wchar_t breakBuffer[10];
        swprintf_s(breakBuffer, 10, L"%d", g_app.tempBreakMinutes);
        g_app.hBreakEdit = CreateWindowW(
            L"EDIT", breakBuffer,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
            10, 90, 180, 25,
            g_app.hWnd, (HMENU)ID_BREAK_EDIT, GetModuleHandle(NULL), NULL
        );
        
        // 保存按钮
        g_app.hSaveButton = CreateWindowW(
            L"BUTTON", L"保存",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 125, 85, 30,
            g_app.hWnd, (HMENU)ID_SAVE_BUTTON, GetModuleHandle(NULL), NULL
        );
        
        // 取消按钮
        g_app.hCancelButton = CreateWindowW(
            L"BUTTON", L"取消",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            105, 125, 85, 30,
            g_app.hWnd, (HMENU)ID_CANCEL_BUTTON, GetModuleHandle(NULL), NULL
        );
        
        // 设置字体
        HFONT hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        
        SendMessageW(g_app.hWorkLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(g_app.hBreakLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(g_app.hWorkEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(g_app.hBreakEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(g_app.hSaveButton, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(g_app.hCancelButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    } else {
        // 显示已存在的控件
        ShowWindow(g_app.hWorkLabel, SW_SHOW);
        ShowWindow(g_app.hBreakLabel, SW_SHOW);
        ShowWindow(g_app.hWorkEdit, SW_SHOW);
        ShowWindow(g_app.hBreakEdit, SW_SHOW);
        ShowWindow(g_app.hSaveButton, SW_SHOW);
        ShowWindow(g_app.hCancelButton, SW_SHOW);
        
        // 更新输入框的值
        wchar_t workBuffer[10];
        wchar_t breakBuffer[10];
        swprintf_s(workBuffer, 10, L"%d", g_app.tempWorkMinutes);
        swprintf_s(breakBuffer, 10, L"%d", g_app.tempBreakMinutes);
        SetWindowTextW(g_app.hWorkEdit, workBuffer);
        SetWindowTextW(g_app.hBreakEdit, breakBuffer);
    }
}

// 保存设置到INI文件
void SaveSettingsToINI() {
    wchar_t iniPath[MAX_PATH];
    GetModuleFileNameW(NULL, iniPath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(iniPath, L'\\');
    if (lastSlash) {
        *(lastSlash + 1) = L'\0';
        wcscat_s(iniPath, MAX_PATH, L"pomodoro_settings.ini");
    }
    
    wchar_t workStr[16], breakStr[16];
    swprintf_s(workStr, 16, L"%d", g_app.tempWorkMinutes);
    swprintf_s(breakStr, 16, L"%d", g_app.tempBreakMinutes);
    
    WritePrivateProfileStringW(L"Settings", L"WorkMinutes", workStr, iniPath);
    WritePrivateProfileStringW(L"Settings", L"BreakMinutes", breakStr, iniPath);
}

// 从INI文件加载设置
void LoadSettingsFromINI() {
    wchar_t iniPath[MAX_PATH];
    GetModuleFileNameW(NULL, iniPath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(iniPath, L'\\');
    if (lastSlash) {
        *(lastSlash + 1) = L'\0';
        wcscat_s(iniPath, MAX_PATH, L"pomodoro_settings.ini");
    }
    
    int workMinutes = GetPrivateProfileIntW(L"Settings", L"WorkMinutes", 27, iniPath);
    int breakMinutes = GetPrivateProfileIntW(L"Settings", L"BreakMinutes", 3, iniPath);
    
    // 验证加载的值
    if (workMinutes > 0 && workMinutes <= 120 && breakMinutes > 0 && breakMinutes <= 60) {
        g_app.tempWorkMinutes = workMinutes;
        g_app.tempBreakMinutes = breakMinutes;
        g_app.timer.workDuration = workMinutes * 60;
        g_app.timer.breakDuration = breakMinutes * 60;
        g_app.timer.remainingTime = g_app.timer.workDuration;
    }
}

// 保存设置
void SaveSettings() {
    wchar_t workBuffer[10];
    wchar_t breakBuffer[10];
    
    GetWindowTextW(g_app.hWorkEdit, workBuffer, 10);
    GetWindowTextW(g_app.hBreakEdit, breakBuffer, 10);
    
    int workMinutes = _wtoi(workBuffer);
    int breakMinutes = _wtoi(breakBuffer);
    
    // 验证输入
    if (workMinutes <= 0 || workMinutes > 120 || breakMinutes <= 0 || breakMinutes > 60) {
        MessageBoxW(g_app.hWnd, L"请输入有效的时间范围（工作: 1-120分钟，休息: 1-60分钟）", L"输入错误", MB_OK | MB_ICONWARNING);
        return;
    }
    
    // 保存设置
    g_app.tempWorkMinutes = workMinutes;
    g_app.tempBreakMinutes = breakMinutes;
    
    // 更新计时器状态
    g_app.timer.workDuration = workMinutes * 60;
    g_app.timer.breakDuration = breakMinutes * 60;
    
    // 如果当前不在运行状态，更新剩余时间
    if (!g_app.timer.isRunning) {
        g_app.timer.remainingTime = g_app.timer.isWorking ? g_app.timer.workDuration : g_app.timer.breakDuration;
        UpdateTimerDisplay();
    }
    
    // 保存到INI文件
    SaveSettingsToINI();
    
    // 返回主界面
    ShowMainView();
}

// 程序入口点
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);
    
    // 创建主窗口
    CreateMainWindow(hInstance);
    
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}