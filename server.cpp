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

    char recvbuf[1024];
    int recvbuflen = 1024;
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
                }
                if(file_exists(repo))
                {
                    cout << "Clone Repository done.." << endl;
                    string str = "repos\\" + fsw_name + "\\" + fsw_name + ".tk";
                    const fs::path image{ str};
                    if(file_exists(image))
                    {
                        ifstream sourceFile(str, std::ios::binary);
                        ofstream destinationFile("BinaryFile.bin", std::ios::binary);
                        destinationFile << sourceFile.rdbuf();
                        sourceFile.close();
                        destinationFile.close();
                        //system("LoaderStart.bat");
                        STARTUPINFO si;
                        PROCESS_INFORMATION pi;

                        ZeroMemory(&si, sizeof(si));
                        si.cb = sizeof(si);
                        ZeroMemory(&pi, sizeof(pi));

                        if(CreateProcess(NULL, "LoaderStart.bat", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
                        {
                            std::cout << "Process started!" << std::endl;
                            WaitForSingleObject(pi.hProcess, INFINITE);
                            CloseHandle(pi.hProcess);
                            CloseHandle(pi.hThread);
                        }
                        else
                        {
                            std::cout << "Failed to start process!" << std::endl;
                        }
                    }
                    else
                    {
                        cout << "Repository TK File Found!!" << endl;
                    }
                }
                else
                {
                    cout << "Repository Not Found!!" << endl;
                }
            }
            else
            {
                cout << "Get the wrong FSW name!!" << endl;
            }
        }
    }

    //closesocket(clientSocket);
    cout << "Soket kapatiliyor.." << endl;
    closesocket(listenSocket);
    WSACleanup();

    return 0;
}
