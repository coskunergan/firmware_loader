#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include <filesystem>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>

using namespace std;
namespace fs = std::filesystem;

/**********************************************************************************************/
/**********************************************************************************************/
/**********************************************************************************************/

bool file_exists(const fs::path &p, fs::file_status s = fs::file_status{})
{
    std::cout << p;
    if(fs::status_known(s) ? fs::exists(s) : fs::exists(p))
    {
        std::cout << " exists\n";
        return true;
    }
    std::cout << " does not exist\n";
    return false;
}

/**********************************************************************************************/

int send_response(int fd, const char *header, const char *content_type, const char *body)
{
    const int max_response_size = 65536;
    char response[max_response_size];

    // Get current time for the HTTP header
    time_t t1 = time(NULL);
    struct tm *ltime = localtime(&t1);

    // How many bytes in the body
    int content_length = strlen(body);

    int response_length = sprintf(response,
                                  "%s\r\n"
                                  "Access-Control-Allow-Origin: %s\r\n"
                                  "Access-Control-Allow-Methods: %s\r\n"
                                  "Content-Length: %d\r\n"
                                  "Content-Type: %s\r\n"
                                  "Date: %s" // asctime adds its own newline
                                  "Connection: close\r\n"
                                  "\r\n" // End of HTTP header
                                  "%s",

                                  header,
                                  "*",
                                  "POST, GET",
                                  content_length,
                                  content_type,
                                  asctime(ltime),
                                  body
                                 );

    // Send it all!
    int rv = send(fd, response, response_length, 0);

    if(rv < 0)
    {
        perror("send");
    }

    return rv;
}

/**********************************************************************************************/

int main()
{
    // winsock başlatma
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(iResult != 0)
    {
        cout << "WSAStartup failed: " << iResult << endl;
        return 1;
    }

    // soket oluşturma
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(listenSocket == INVALID_SOCKET)
    {
        cout << "socket failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // soketi adres ve porta bağlama
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(8080);
    if(bind(listenSocket, (SOCKADDR *)&address, sizeof(address)) == SOCKET_ERROR)
    {
        cout << "bind failed: " << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // dinleme moduna geç
    if(listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        cout << "listen failed: " << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    cout << "Sunucu dinlemede, 8080 portunda..." << endl;

    char recvbuf[20];
    int recvbuflen = 20;
    string myString{};
    SOCKET clientSocket;

    while(true)
    {
        clientSocket = accept(listenSocket, NULL, NULL);
        if(clientSocket == INVALID_SOCKET)
        {
            cout << "accept failed: " << WSAGetLastError() << endl;
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        if(recv(clientSocket, recvbuf, recvbuflen, 0) > 0)
        {
            cout << "Sunucudan gelen veri: " << recvbuf << endl;
            shutdown(listenSocket, SD_RECEIVE);
            string response_str{};
            myString = recvbuf;
            size_t found = myString.find("FSW");
            if(found != string::npos)
            {
                string fsw_name = myString.substr(found, 10);
                cout << "\n Alinan anahtar [" + fsw_name + "]\n"  << endl;
                const fs::path repo{"repos/" + fsw_name};
                if(file_exists(repo))
                {
                    string a = "rmdir /s /q repos\\" + fsw_name;
                    system(a.c_str()); // delete folder!
                    cout << "File deleted.." << endl;
                }
                if(!file_exists(repo))
                {
                    string str = "clone_repository.exe http://uretim:123456@10.15.16.10/Bonobo.Git.Server/" + fsw_name + ".git repos/" + fsw_name;
                    system(str.c_str());
                }
                else
                {
                    cout << "File not deleted!!" << endl;
                    response_str = "ERROR:5";
                }
                if(file_exists(repo))
                {
                    cout << "Clone Repository done.." << endl;
                    string str = "repos\\" + fsw_name + "\\" + fsw_name + ".tk";                    
                    const fs::path image{ str};
                    if(file_exists(image))
                    {
                        ifstream sourceFile(str, ios::binary);
                        ofstream destinationFile("BinaryFile.bin", ios::binary);
                        destinationFile << sourceFile.rdbuf();
                        sourceFile.close();
                        destinationFile.close();
                        STARTUPINFO si;
                        PROCESS_INFORMATION pi;
                        ZeroMemory(&si, sizeof(si));
                        si.cb = sizeof(si);
                        ZeroMemory(&pi, sizeof(pi));
                        if(CreateProcess(NULL, (char *)"LoaderStart.bat", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
                        {
                            cout << "Process started!" << endl;
                            WaitForSingleObject(pi.hProcess, INFINITE);
                            CloseHandle(pi.hProcess);
                            CloseHandle(pi.hThread);
                            response_str = "SUCCES";
                            string fileContent, comparisonString;
                            ifstream myfile("result.txt", ios::in);
                            if(myfile.is_open())
                            {
                                getline(myfile, fileContent);
                                myfile.close();
                                if(fileContent == "okey")
                                {
                                    response_str = "SUCCES";
                                }
                                else if(fileContent == "fail")
                                {
                                    response_str = "ERROR:9";
                                }
                                else if(fileContent == "burn")
                                {
                                    response_str = "ERROR:8";
                                }
                                else if(fileContent == "tout")
                                {
                                    response_str = "ERROR:7";
                                }
                                else
                                {
                                    response_str = "ERROR:6";
                                }
                            }
                            else
                            {
                                cout << "Unable to open file";
                                response_str = "ERROR:5";
                            }
                        }
                        else
                        {
                            cout << "Failed to start process!" << endl;
                            response_str = "ERROR:4";
                        }
                    }
                    else
                    {
                        cout << "Repository TK File Found!!" << endl;
                        response_str = "ERROR:3";
                    }
                }
                else
                {
                    cout << "Repository Not Found!!" << endl;
                    response_str = "ERROR:2";
                }
            }
            else
            {
                cout << "Get the wrong FSW name!!" << endl;
                response_str = "ERROR:1";
            }            
            send_response(clientSocket, "HTTP/1.1 200 OK", "text/plain", response_str.c_str());
            fflush(stdout);
        }
    }

    cout << "Soket kapatiliyor.." << endl;
    closesocket(listenSocket);
    WSACleanup();

    return 0;
}
