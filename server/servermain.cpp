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
					StaticDocumentEndPoint{"index.html"}
				},
				{
					"log",
					StaticDocumentEndPoint{"log.html"}
				},
				{
					"screenshot",
					ApiEndPoint{[](HttpRequestContext& context)
					{
						std::string userName = context.GetPathStep();
						if (userName == "")
							throw http::status::not_found;
						context.responseStream << "<!doctype html>" << std::endl;
						context.responseStream << "<html>" << std::endl;
						context.responseStream << "  <head>" << std::endl;
						context.responseStream << "    <style> .small { max-width: 100px; height: auto; } </style>" << std::endl;
						context.responseStream << "    <title>screenshot</title>" << std::endl;
						context.responseStream << "    <script type = \"text/javascript\">" << std::endl;
						context.responseStream << "      let page = {};" << std::endl;
						context.responseStream << "      page.SwitchSize = function (id) {" << std::endl;
						context.responseStream << "        const imgElement = document.getElementById(id);" << std::endl;
						context.responseStream << "        if(imgElement.className == 'small')" << std::endl;
						context.responseStream << "          imgElement.className = '';" << std::endl;
						context.responseStream << "        else" << std::endl;
						context.responseStream << "          imgElement.className = 'small';" << std::endl;
						context.responseStream << "        return false;" << std::endl;
						context.responseStream << "      };" << std::endl;
						context.responseStream << "      page.CreateRequest = function () {" << std::endl;
						context.responseStream << "        let Request = false;" << std::endl;
						context.responseStream << "        if (window.XMLHttpRequest) {" << std::endl;
						context.responseStream << "          Request = new XMLHttpRequest();" << std::endl;
						context.responseStream << "        }" << std::endl;
						context.responseStream << "        else if (window.ActiveXObject) {" << std::endl;
						context.responseStream << "          try {" << std::endl;
						context.responseStream << "            Request = new ActiveXObject(\"Microsoft.XMLHTTP\");" << std::endl;
						context.responseStream << "          }" << std::endl;
						context.responseStream << "          catch (CatchException) {" << std::endl;
						context.responseStream << "            Request = new ActiveXObject(\"Msxml2.XMLHTTP\");" << std::endl;
						context.responseStream << "          }" << std::endl;
						context.responseStream << "        }" << std::endl;
						context.responseStream << "        if (!Request) {" << std::endl;
						context.responseStream << "          console.log(\"cant create request\");" << std::endl;
						context.responseStream << "        }" << std::endl;
						context.responseStream << "        return Request;" << std::endl;
						context.responseStream << "      };" << std::endl;
						context.responseStream << "      page.GetApiResult = function(url, onCompete) {" << std::endl;
						context.responseStream << "        const request = page.CreateRequest();" << std::endl;
						context.responseStream << "        request.onreadystatechange = function () {" << std::endl;
						context.responseStream << "          if (request.readyState == 4) {" << std::endl;
						context.responseStream << "            onCompete(JSON.parse(request.responseText));" << std::endl;
						context.responseStream << "          }" << std::endl;
						context.responseStream << "        }" << std::endl;
						context.responseStream << "        request.open(\"get\", url, true);" << std::endl;
						context.responseStream << "        request.send(null);" << std::endl;
						context.responseStream << "      }" << std::endl;
						context.responseStream << "      document.addEventListener(\"DOMContentLoaded\", function() {" << std::endl;
						context.responseStream << "        page.GetApiResult(\"/api/monitorscount/" << userName << "\", function(result) {" << std::endl;
						context.responseStream << "          let innerHtml = \"\";" << std::endl;
						context.responseStream << "          for(let i = 0; i < result.monitorsCount; i++) {" << std::endl;
						context.responseStream << "            innerHtml += \"<img id=\\\"img\" + i + \"\\\" class=\\\"small\\\" src = \\\"/api/takescreenshot/" << userName << "/\" + i + \"\\\" onclick=\\\"page.SwitchSize('img\" + i + \"')\\\">\";" << std::endl;
						context.responseStream << "          }" << std::endl;
						context.responseStream << "          const container = document.getElementById(\"screenshots-div\");" << std::endl;
						context.responseStream << "          container.innerHTML = innerHtml;" << std::endl;
						context.responseStream << "        });" << std::endl;
						context.responseStream << "        return false;" << std::endl;
						context.responseStream << "      });" << std::endl;
						context.responseStream << "    </script>" << std::endl;
						context.responseStream << "  </head>" << std::endl;
						context.responseStream << "  <body>" << std::endl;
						context.responseStream << "    <a href=\"/\">active sessions</a> <a href=\"/log\">sessions log</a><br>" << std::endl;
						context.responseStream << "    <div id=\"screenshots-div\"></div>" << std::endl;
						context.responseStream << "  </body>" << std::endl;
						context.responseStream << "</html>" << std::endl;
					}}
				},
				{
					"api",
					Router
					{
						{
							"login",
							ApiEndPoint{[](HttpRequestContext& context)
							{
								std::string userName = context.GetPathStep();
								time_point now = time_point::clock::now();
								Session session{ now , now, context.remote_address, false };
								activeSessions[userName] = session;
								std::cout << "user " << userName << " logged in" << std::endl;
							}}
						},
						{
							"logout",
							ApiEndPoint{[](HttpRequestContext& context)
							{
								std::string userName = context.GetPathStep();
								auto findResultIterator = activeSessions.find(userName);
								if (findResultIterator == activeSessions.end())
									throw http::status::not_found;
								Session session = findResultIterator->second;
								activeSessions.erase(userName);
								session.stopped = time_point::clock::now();
								session.finished = true;
								sessionsLog[userName].insert(session);
								std::cout << "user " << userName << " logged out" << std::endl;
							}}
						},
						{
							"userlog",
							ApiEndPoint{[](HttpRequestContext& context)
							{
								std::string userName = context.GetPathStep();
								auto findResultIterator = sessionsLog.find(userName);
								if (findResultIterator == sessionsLog.end())
									throw http::status::not_found;
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
							}}
						},
						{
							"sessionslog",
							ApiEndPoint{[](HttpRequestContext& context)
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
							}}
						},
						{
							"activesessions",
							ApiEndPoint{[](HttpRequestContext& context)
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
							}}
						},
						{
							"monitorscount",
							ApiEndPoint{[](HttpRequestContext& context)
							{
								std::string userName = context.GetPathStep();
								auto findResultIterator = activeSessions.find(userName);
								if (findResultIterator == activeSessions.end())
									throw http::status::not_found;
								std::string address = findResultIterator->second.address;
								HttpRequest request{ address, std::to_string(clientPort), "/monitorscount", 11 };
								http::response<http::dynamic_body> clientMachineResponse{ std::move(request.Get()) };
								context.responseStream << boost::beast::buffers_to_string(clientMachineResponse.body().data());
							}}
						},
						{
							"takescreenshot",
							ApiEndPoint{[](HttpRequestContext& context)
							{
								std::string userName = context.GetPathStep();
								auto findResultIterator = activeSessions.find(userName);
								if (findResultIterator == activeSessions.end())
									throw http::status::not_found;
								std::string address = findResultIterator->second.address;

								HttpRequest request{ address, std::to_string(clientPort), "/takescreenshot/" + context.GetPathStep(), 11 };
								http::response<http::dynamic_body> clientMachineResponse{ std::move(request.Get()) };
								context.headers[http::field::content_disposition] = "attachment; filename = img.bmp";
								context.headers[http::field::content_type] = "image/bmp";
								context.headers[http::field::content_transfer_encoding] = "binary";
								context.headers[http::field::accept_ranges] = "bytes";
								context.responseStream << boost::beast::buffers_to_string(clientMachineResponse.body().data());
							}}
						}
					}
				},
				{
					"favicon.ico",
					ApiEndPoint{[](HttpRequestContext&) {}}
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