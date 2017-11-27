// TestHMD.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <thread>
#include <string>
#include <sstream>
#include <iostream>
#include <memory>
#include <vector>
#include <conio.h>  
#include <tlhelp32.h>
#include <ppltasks.h>

using namespace Concurrency;
using namespace Windows::Foundation;
using namespace Windows::System;

#define MR_APP_NAME L"TestHMDApp.exe"
#define MR_APP_PROTOCOL L"testhmdapp:"
#define MR_PORTAL_PROTOCOL L"ms-holographicfirstrun:"

class TestHMD
{
public:
    TestHMD()
        :m_quitting(false)
        ,m_thread(nullptr)
    {
    }

    ~TestHMD() 
    {
        Stop();
    }

    void Start() 
    { 
        m_quitting = false;
        m_thread = std::make_shared<std::thread>(&ThreadProc, this);
    }

    void Stop()
    {
        if(m_thread != nullptr)
        { 
            m_quitting = true;
            m_thread->join();
            m_thread.reset();
        }
    }

private:
    static void ThreadProc(TestHMD* h)
    {
        h->ThreadWorker();
    }

    void LaunchUWPApp(Platform::String^ protocol)
    {
        // Launch the Win32 App
        auto uri = ref new Uri(protocol); // The protocol handled by the launched app
        auto options = ref new LauncherOptions();
        concurrency::task<bool> task(Launcher::LaunchUriAsync(uri, options));
    }

    void ThreadWorker()
    {
        DWORD id = 0;
        if (!IsProcessRunning(L"MixedRealityPortal.exe", id))
        {
            std::cout << "TestHMD:starting MR Portal..." << std::endl;
            LaunchUWPApp(MR_PORTAL_PROTOCOL);
            Sleep(10000);
            std::cout << "TestHMD:starting MR App..." << std::endl;
            SendMRPortalEvents();
            LaunchUWPApp(MR_APP_PROTOCOL);
        }

        if (IsProcessRunning(L"ApplicationFrameHost.exe", id))
        {
            MaximizeAppWindow(id);
        }

        if (IsProcessRunning(L"MixedRealityPortal.exe", id))
        {
            MaximizeAppWindow(id);
        }

        while (!m_quitting)
        {
            std::stringstream w;

            Sleep(1000);

            DWORD mrAppId = 0;
            bool mrAppRunning = IsProcessRunning(MR_APP_NAME, mrAppId);
            if (mrAppRunning)
            {
                std::cout << "MR App is running." << std::endl;
                continue;
            }
            else
            {
                w << "MR App is not running, ";
            }

            HWND hWnd = GetForegroundWindow();
            DWORD foregroundAppId = 0;
            GetWindowThreadProcessId(hWnd, &foregroundAppId);

            DWORD holoAppId = 0;
            bool holoAppRunning = IsProcessRunning(L"HoloShellApp.exe", holoAppId);

            DWORD portalAppId = 0;
            bool portalRunning = IsProcessRunning(L"MixedRealityPortal.exe", portalAppId);

            // w << "Foreground App:" << foregroundAppId << " HoloApp: " << holoAppId << ", ";
            if (holoAppRunning && (foregroundAppId == holoAppId))
            {
                w << "HMD is on, ";
                if (!mrAppRunning)
                {
                    w << "Launching MR App, ";
                    LaunchUWPApp(MR_APP_PROTOCOL);
                }
            }
            else
            {
                w << "HMD is off, ";
            }

            if (portalRunning)
            {
                w << "portal is running, ";
            }
            else
            {
                w << "portal is not running, ";
            }

            std::cout << w.str() << std::endl;
        }
        std::cout << "TestHMD:threadworker exited" << std::endl;
    }

    bool IsProcessRunning(const wchar_t *processName, DWORD& id)
    {
        bool exists = false;
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

        if (Process32First(snapshot, &entry))
        {
            while (Process32Next(snapshot, &entry))
            {
                if (!_wcsicmp(entry.szExeFile, processName))
                {
                    exists = true;
                    id = entry.th32ProcessID;
                }
            }
        }

        CloseHandle(snapshot);
        return exists;
    }

    void SendKeyboardEvents(const std::vector<WORD>& keys)
    {
        INPUT ip = { 0 };
        ip.type = INPUT_KEYBOARD;

        for (const WORD& i : keys)
        {
            ip.ki.wVk = VK_LWIN;
            ip.ki.dwFlags = 0;
            SendInput(1, &ip, sizeof(INPUT));
        }

        // reverse iterate through vector to send Keyup events
        auto rit = keys.rbegin();
        for (; rit != keys.rend(); ++rit)
        {
            ip.ki.wVk = *rit;
            ip.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &ip, sizeof(INPUT));
        }
    }

    void SendMouseClick(LONG x, LONG y)
    {
        INPUT ip = { 0 };
        ip.type = INPUT_MOUSE;
        ip.mi.dx = x * (65536 / GetSystemMetrics(SM_CXSCREEN)); //x being coord in pixels
        ip.mi.dy = y * (65536 / GetSystemMetrics(SM_CYSCREEN)); //y being coord in pixels
        ip.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
        SendInput(1, &ip, sizeof(INPUT));
        ip.mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE;
        SendInput(1, &ip, sizeof(INPUT));
        ip.mi.dwFlags = MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE;
        SendInput(1, &ip, sizeof(INPUT));
    }


    void SendMRPortalEvents()
    {
        // Send Win-Y twice to release MR Portal's focus
        SendKeyboardEvents({ VK_LWIN, 0x59 });
        SendKeyboardEvents({ VK_LWIN, 0x59 });
        SendMouseClick(2393, 35);
        Sleep(1000);
        SendMouseClick(2910, 1011);
    }

    void MaximizeAppWindow(DWORD id)
    {
        std::vector<HWND> processWindows;
        GetProcessMainWindows(id, processWindows);
        for (const HWND& hWnd : processWindows)
        {
            RECT rect;
            GetWindowRect(hWnd, &rect);
            auto l = rect.left;
            auto t = rect.top;
            auto r = rect.right;
            auto b = rect.bottom;

        }
    }

    void GetProcessMainWindows(DWORD dwProcessID, std::vector<HWND> &vWindows)
    {
        HWND hwnd = NULL;
        do
        {
            hwnd = FindWindowEx(NULL, hwnd, NULL, NULL);
            DWORD dwPID = 0;
            GetWindowThreadProcessId(hwnd, &dwPID);
            if (dwPID == dwProcessID)
            {
                vWindows.push_back(hwnd);
            }
        } while (hwnd != NULL);
    }

    std::shared_ptr<std::thread> m_thread;
    bool m_quitting;
};

[Platform::MTAThread]
int main()
{
    TestHMD app;
    std::cout << "Press q to exit app..." << std::endl;
    app.Start();
    while (auto c = _getch() != 'q');
    app.Stop();
    return 0;
}

