#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <utility>
#include "VersionChecker.h"

using namespace std;

VersionChecker::VersionChecker()
{
    WSADATA wsaData;   

    if (WSAStartup(MAKEWORD(2,0), &wsaData) != 0)
        inited = false;
    else
        inited = true;
}

VersionChecker::~VersionChecker()
{
    WSACleanup();
}

wstring VersionChecker::IsNewVersion(wstring curvers)
{
    if (!inited)
        return L"";	

    int sockfd=0, numbytes;  
    vector<char> buf(MAXDATASIZE);
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo("www.code.google.com", "80", &hints, &servinfo)) != 0) {
        return L"";
    }

    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;
        
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            closesocket(sockfd);
            continue;
        }
        break;
    }

    if (p == NULL) 
    {
        closesocket(sockfd);
        return L"";
    }	

    freeaddrinfo(servinfo); 

    struct timeval tv;
    tv.tv_sec = 25000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv);

    if (send(sockfd, "GET http://win7shell.googlecode.com/svn/wiki/Version.wiki HTTP/1.1\n\n", 
        strlen("GET http://win7shell.googlecode.com/svn/wiki/Version.wiki HTTP/1.1\n\n"), 0) == SOCKET_ERROR)
    {
        closesocket(sockfd);
        return L"";
    }

    if ((numbytes = recv(sockfd, &buf[0], MAXDATASIZE-1, 0)) == -1) 
    {
        closesocket(sockfd);
        return L"";
    }

    wstring GET;
    try 
    {
        GET.assign(buf.begin(), buf.begin()+numbytes);
    }
    catch (std::exception e)
    {
        MessageBoxA(0, e.what(), "Error", MB_ICONSTOP);
        closesocket(sockfd);
        return L"";
    }

    if (GET.substr(0, 15) != L"HTTP/1.1 200 OK")
    {
        closesocket(sockfd);
        return L"";
    }

    size_t pos = GET.find(L"\r\n\r\n", 0);
    if (pos != string::npos)
        GET.erase(0, pos+22);

    pos = GET.find(L"\xA\xA", 0);
    if (pos == string::npos)
    {
        closesocket(sockfd);
        return L"";
    }

    if (curvers == GET.substr(1, pos-1) || (_wcsicmp(curvers.c_str(), GET.substr(1, pos-1).c_str()) >= 0))
    {
        closesocket(sockfd);
        return L"";
    }

    closesocket(sockfd);

    return GET;
}

int VersionChecker::GetMessages(std::list<std::pair<std::wstring, std::wstring>>& message_list, int &last_message)
{
    if (!inited)
        return -1;

    int sockfd=0, numbytes;  
    vector<char> buf(MAXDATASIZE);
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo("www.code.google.com", "80", &hints, &servinfo)) != 0) {
        return -1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            closesocket(sockfd);
            continue;
        }
        break;
    }

    if (p == NULL) 
    {
        closesocket(sockfd);
        return -1;
    }	

    freeaddrinfo(servinfo); 

    struct timeval tv;
    tv.tv_sec = 25000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv);

    if (send(sockfd, "GET http://win7shell.googlecode.com/svn/wiki/Messages.wiki HTTP/1.1\n\n", 
        strlen("GET http://win7shell.googlecode.com/svn/wiki/Messages.wiki HTTP/1.1\n\n"), 0) == SOCKET_ERROR)
    {
        closesocket(sockfd);
        return -1;
    }

    if ((numbytes = recv(sockfd, &buf[0], MAXDATASIZE-1, 0)) == -1) 
    {
        closesocket(sockfd);
        return -1;
    }

    // Parse header
    wstring GET;
    try 
    {
        GET.assign(buf.begin(), buf.begin()+numbytes);
    }
    catch (std::exception e)
    {
        MessageBoxA(0, e.what(), "Error", MB_ICONSTOP);
        closesocket(sockfd);
        return -1;
    }

    if (GET.substr(0, 15) != L"HTTP/1.1 200 OK")
    {
        closesocket(sockfd);
        return -1;
    }

    size_t pos = GET.find(L"\r\n\r\n", 0);
    if (pos != string::npos)
        GET.erase(0, pos+22);

    int first_message = MAXINT;
    int last_ID = -1;
    std::pair<std::wstring, std::wstring> message;

    message_list.clear();

    // Parse messages
    pos = 0;
    while (pos != std::wstring::npos) 
    {   
        if (GET[pos+1] == L'#')
        {
            size_t c_pos = GET.find(L" ", pos+1);
            UINT ID = _wtoi(GET.substr(pos+2, c_pos).c_str());

            if (ID <= last_message)
            {
                pos = GET.find(L"\n#", pos+2);
                continue;
            }
            
            if (ID < first_message)
            {
                first_message = ID;
            }
            
            message.second = L"";
            pos = GET.find(L"\n#", pos+2);

            if (last_ID != ID) // new message
            {   
                message.first = GET.substr(c_pos+1, pos-c_pos-1);
                last_ID = ID;
            }
            else // link
            {
                message.second = GET.substr(c_pos+1, pos-c_pos-1);
            }

            if (pos == std::wstring::npos || !message.second.empty())
            {
                last_message = ID;
                message_list.push_back(message);
                message.first = L"";
                message.second = L"";
            }
        }
    } 

    closesocket(sockfd);

    return first_message;
}