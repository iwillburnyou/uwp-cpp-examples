// TestHMD.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <thread>
#include <string>
#include <sstream>
#include <iostream>
#include <memory>
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
        , m_thread(nullptr)
    {
    }

    ~TestHMD() 
    {
        m_quitting = true;
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
            LaunchUWPApp(MR_APP_PROTOCOL);
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

