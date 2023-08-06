#define _WIN32_WINNT 0x0601
#include <string>
#include <map>
#include <string>
#include <set>
#include <list>
#include <cstdlib>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "session.h"
#include "../webserverlib/webserver.h"
#include "../httpclient.h"

using namespace webserverlib;

std::map<std::string, std::set<Session>> sessionsLog;
std::map<std::string, Session> activeSessions;
unsigned short listenPort;
unsigned short clientPort;
std::string bindAddress;

enum ApiMethodId
{
	login,
	logout,
	userlog,
	sessionslog,
	activesessions,
	takescreenshot
};

std::map<std::string, ApiMethodId> methodsIds
{
	{"login", ApiMethodId::login},
	{"logout", ApiMethodId::logout},
	{"userlog", ApiMethodId::userlog},
	{"sessionslog", ApiMethodId::sessionslog},
	{"activesessions", ApiMethodId::activesessions},
	{"takescreenshot", ApiMethodId::takescreenshot}
};

int main()
{
	std::stringstream buffer;

	{
		std::ifstream ifconfigfile("serverconfig.json");
		buffer << ifconfigfile.rdbuf();
		boost::property_tree::ptree configJsonRoot;
		boost::property_tree::read_json(buffer, configJsonRoot);
		bindAddress = configJsonRoot.get<std::string>("bindaddress");
		listenPort = configJsonRoot.get<unsigned short>("listenport");
		clientPort = configJsonRoot.get<unsigned short>("clientport");
	}

	WebServer webServer(
		WebServer::SetPort(listenPort),
		WebServer::SetAddress(bindAddress),
		WebServer::SetRouter(
			Router
			{
				{
					"",
					StaticDocumentEndPoint("index.html")
				},
				{
					"log",
					StaticDocumentEndPoint("log.html")
				},
				{
					"screenshot",
					ApiEndPoint([](HttpRequestContext& context)
					{
						std::string userName = context.GetPathStep();
						if (userName == "")
						{
							context.responseStream << "404";
							return;
						}
						context.responseStream << "<!doctype html><html><head><title></title></head>";
						context.responseStream << "<body><a href=\"/\">active sessions</a> <a href=\"/log\">sessions log</a><br>";
						context.responseStream << "<img src=\"/api/takescreenshot/" << userName << "\"></body></html>";
					})
				},
				{
					"api",
					ApiEndPoint([](HttpRequestContext& context)
					{
						auto i = methodsIds.find(context.GetPathStep());
						if (i == methodsIds.end())
						{
							context.responseStream << "404";
							return;
						}
						switch (i->second)
						{
						case ApiMethodId::login:
						{
							std::string userName = context.GetPathStep();
							time_point now = time_point::clock::now();
							Session session{ now , now, context.remote_address, false };
							activeSessions[userName] = session;
							std::cout << "user " << userName << " logged in" << std::endl;
							break;
						}
						case ApiMethodId::logout:
						{
							std::string userName = context.GetPathStep();
							Session session = activeSessions[userName];
							activeSessions.erase(userName);
							session.stopped = time_point::clock::now();
							session.finished = true;
							sessionsLog[userName].insert(session);
							std::cout << "user " << userName << " logged out" << std::endl;
							break;
						}
						case ApiMethodId::sessionslog:
						{
							std::stringstream& stream = context.responseStream;
							context.responseStream << "[ ";
							for (auto logUsersIterator = sessionsLog.begin(); logUsersIterator != sessionsLog.end(); logUsersIterator++)
							{
								if (logUsersIterator != sessionsLog.begin())
									stream << ", ";
								stream << "{ \"username\" : \"" << logUsersIterator->first
									<< "\", \"sessions\" : [ ";
								std::set<Session>& sessions = logUsersIterator->second;
								for (auto sessionsIterator = sessions.begin(); sessionsIterator != sessions.end(); sessionsIterator++)
								{
									if (sessionsIterator != sessions.begin())
										stream << ", ";
									stream << *sessionsIterator;
								}
								stream << " ] }";
							}
							context.responseStream << " ]";
							break;
						}
						case ApiMethodId::activesessions:
						{
							std::stringstream& stream = context.responseStream;
							context.responseStream << "[ ";
							for (auto activeUsersIterator = activeSessions.begin(); activeUsersIterator != activeSessions.end(); activeUsersIterator++)
							{
								if (activeUsersIterator != activeSessions.begin())
									stream << ", ";
								stream << "{ \"username\" : \"" << activeUsersIterator->first
									<< "\", \"session\" : " << activeUsersIterator->second << " }";
							}
							context.responseStream << " ]";
							break;
						}
						case ApiMethodId::userlog:
						{
							std::string userName = context.GetPathStep();
							auto findResultIterator = sessionsLog.find(userName);
							if (findResultIterator == sessionsLog.end())
							{
								//404 or bad request or something else
								break;
							}
							std::stringstream& stream = context.responseStream;
							stream << "[ ";
							std::set<Session>& sessions = findResultIterator->second;
							for (auto sessionsIterator = sessions.begin(); sessionsIterator != sessions.end(); sessionsIterator++)
							{
								if (sessionsIterator != sessions.begin())
									stream << ", ";
								stream << *sessionsIterator;
							}
							stream << " ]";
							break;
						}
						case ApiMethodId::takescreenshot:
						{
							std::string userName = context.GetPathStep();
							auto findResultIterator = activeSessions.find(userName);
							if (findResultIterator == activeSessions.end())
							{
								//404 or bad request or something else
								break;
							}
							std::string address = findResultIterator->second.address;
							HttpRequest request{ address, std::to_string(clientPort), "/takescreenshot", 11 };
							http::response<http::dynamic_body> clientMachineResponse{ std::move(request.Get()) };
							context.headers[http::field::content_disposition] = "attachment; filename = img.bmp";
							context.headers[http::field::content_type] = "image/bmp";
							context.headers[http::field::content_transfer_encoding] = "binary";
							context.headers[http::field::accept_ranges] = "bytes";
							context.responseStream << boost::beast::buffers_to_string(clientMachineResponse.body().data());
							break;
						}
						default:
							//404 or bad request or something else
							break;
						}
					})
				}
			}));
	
	while (true)
	{
		std::string command;
		std::cin >> command;
		if (command == "exit")
			break;
	}
	return 0;
}