#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include <filesystem>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <unistd.h>

using namespace std;
namespace fs = std::filesystem;

/**********************************************************************************************/
/**********************************************************************************************/
/**********************************************************************************************/

unsigned long crc32(unsigned char *message, int len)
{
    int i, j;
    unsigned long byte, crc, mask;

    i = 0;
    crc = 0xFFFFFFFF;
    while(len--)
    {
        byte = message[i];            // Get next byte.
        crc = crc ^ byte;
        for(j = 7; j >= 0; j--)       // Do eight times.
        {
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
        i = i + 1;
    }
    return ~crc;
}

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
                                  "Result= %s\r\n"
                                  "Connection: close\r\n"
                                  "\r\n" // End of HTTP header
                                  "%s",
                                  header,
                                  "*",
                                  "POST, GET",
                                  content_length,
                                  content_type,
                                  asctime(ltime),
                                  body,
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

int Jlink_Start()
{
    // GDB Server parametreleri
    std::string jlinkGdbServerPath = "C:\\Program Files (x86)\\SEGGER\\JLink_V620c\\JLinkGDBServerCL.exe";
    std::string device = "AT32F421K8T7";
    std::string interfaces = "SWD";
    std::string speed = "1000";

    // GDB Server'ı başlatmak için komutu oluşturun
    std::string command = "\"" + jlinkGdbServerPath + "\" -device " + device + " -if " + interfaces + " -speed " + speed + " -port 2331";

    STARTUPINFO startupInfo = { 0 };
    PROCESS_INFORMATION processInfo = { 0 };

    // GDB Server'ı başlatın
    if(CreateProcess(nullptr, &command[0], nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo) == FALSE)
    {
        std::cout << "GDB Server could not be started." << std::endl;
        return 1;
    }
    std::cout << "kuruldu" << std::endl;

    // GDB Server'ın işlemi tamamlamasını bekleyin
    //WaitForSingleObject(processInfo.hProcess, INFINITE);

    std::cout << "hazir" << std::endl;

    std::string jlinkExePath = "C:\\Program Files (x86)\\SEGGER\\JLink_V620c\\JLink.exe";
    std::string jlinkScriptPath = "flashonly.jlink";

    std::string jlinkCommand = "\"" + jlinkExePath + "\"";
    jlinkCommand += " -device " + device;
    jlinkCommand += " -if " + interfaces;
    jlinkCommand += " -speed " + speed;
    jlinkCommand += " -AutoConnect 1";
    jlinkCommand += " -CommanderScript " + jlinkScriptPath;
    // Uygulama yolu
    LPCSTR applicationPath = jlinkCommand.c_str();

    // Komut satırı parametreleri
    LPCSTR commandLineArgs = "";

    // Gerekli Windows handle'ları
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES saAttr;
    STARTUPINFO siInfo;
    PROCESS_INFORMATION piInfo;

    // Pipe'lar için güvenlik özellikleri ayarları
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Pipe'lar oluşturuluyor
    if(!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0))
    {
        cout << "Hata: Pipe oluşturulamadi." << endl;
        return 1;
    }

    // Uygulama için başlatma bilgileri ayarlanıyor
    ZeroMemory(&siInfo, sizeof(siInfo));
    siInfo.cb = sizeof(siInfo);
    siInfo.hStdError = hWritePipe;
    siInfo.hStdOutput = hWritePipe;
    siInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Uygulama başlatılıyor
    if(!CreateProcess(NULL, (LPSTR)applicationPath, NULL, NULL, TRUE, 0, NULL, NULL, &siInfo, &piInfo))
    {
        cout << "Hata: Uygulama başlatilamadi." << endl;
        return 1;
    }

    std::cout << "----------------------------------------------- " << std::endl;
    // Pipe okuma için hazırlık
    char buffer[1024] = {0};
    DWORD bytesRead;

    int result = 1;

    // Pipe'ları kapatın ve çıktıları okuyun
    CloseHandle(hWritePipe);

    while(ReadFile(hReadPipe, buffer, sizeof(buffer), &bytesRead, NULL))
    {
        cout << buffer << endl;
        if(strstr(buffer, "\r\nO.K.") != nullptr)
        {
            std::cout << ">>>>>>>>>> Yukleme BASARILI <<<<<<<<<<<<<<  " << std::endl;
            result = 0;
            break;
        }
    }
    std::cout << "----------------------------------------------- " << std::endl;

    CloseHandle(hReadPipe);
    CloseHandle(piInfo.hProcess);
    CloseHandle(piInfo.hThread);

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    return result;
}


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
    unsigned long crc = 0;

    //////////////

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
                    string str_tk = "repos\\" + fsw_name + "\\" + fsw_name + ".tk";
                    string str_mtp = "repos\\" + fsw_name + "\\" + fsw_name + ".mtp";
                    string str_art = "repos\\" + fsw_name + "\\" + fsw_name + ".art";
                    const fs::path image_tk{ str_tk };
                    const fs::path image_mtp{ str_mtp };
                    const fs::path image_art{ str_art };
                    if(file_exists(image_tk)) // TK FILE Content
                    {
                        ifstream sourceFile(str_tk, ios::binary);
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
                    else if(file_exists(image_mtp)) // MTP file content
                    {
                        ifstream file(str_mtp, ios::binary);

                        // Get file size
                        file.seekg(0, ios::end);
                        int fileSize = file.tellg();
                        file.seekg(0, ios::beg);

                        // Read file into buffer
                        char *buffer = new char[fileSize];
                        file.read(buffer, fileSize);

                        // Calculate and print CRC32
                        unsigned long crc_file = crc32((unsigned char *)buffer, fileSize);

                        delete[] buffer;

                        file.close();

                        STARTUPINFO si;
                        PROCESS_INFORMATION pi;
                        ZeroMemory(&si, sizeof(si));
                        si.cb = sizeof(si);
                        ZeroMemory(&pi, sizeof(pi));

                        string downloadPrg = "C:\\\\Program Files\\Holtek MCU Development Tools\\HOPE3000\\WCMD.exe -D /F" + str_mtp + " /KICP(E-CON12C)";
                        LPSTR downloadPrgmLpstr = const_cast<LPSTR>(downloadPrg.c_str());

                        const char *burnerPrg = "C:\\\\Program Files\\Holtek MCU Development Tools\\HOPE3000\\WCMD.exe -A";
                        LPSTR burnerPrgmLpstr = const_cast<LPSTR>(burnerPrg);

                        if(crc_file != crc)
                        {
                            cout << "Image differant start the load process!" << endl;
                            //-------------------------
                            if(CreateProcess(NULL,    // No module name (use command line)
                                             downloadPrgmLpstr,        // Command line
                                             NULL,           // Process handle not inheritable
                                             NULL,           // Thread handle not inheritable
                                             FALSE,          // Set handle inheritance to FALSE
                                             0,              // No creation flags
                                             NULL,           // Use parent's environment block
                                             NULL,           // Use parent's starting directory
                                             &si,            // Pointer to STARTUPINFO structure
                                             &pi)           // Pointer to PROCESS_INFORMATION structure
                              )
                            {
                                // Wait until child process exits.
                                WaitForSingleObject(pi.hProcess, 45000);
                                DWORD n = 0;
                                GetExitCodeProcess(pi.hProcess, &n);
                                if(n == 1)
                                {
                                    crc = crc_file;
                                    cout << "Load finish." << endl;
                                }
                                else if(n == 259) // timeout
                                {
                                    crc = 0;
                                    cout << "Load timeout!" << endl;
                                    response_str = "ERROR:12";
                                }
                                else
                                {
                                    crc = 0;
                                    cout << "Load fail!" << endl;
                                    response_str = "ERROR:11";
                                }
                            }
                            else
                            {
                                response_str = "ERROR:10";
                            }
                        }

                        if(crc_file == crc)
                        {
                            CloseHandle(pi.hProcess);
                            if(CreateProcess(NULL,    // No module name (use command line)
                                             burnerPrgmLpstr,        // Command line
                                             NULL,           // Process handle not inheritable
                                             NULL,           // Thread handle not inheritable
                                             FALSE,          // Set handle inheritance to FALSE
                                             0,              // No creation flags
                                             NULL,           // Use parent's environment block
                                             NULL,           // Use parent's starting directory
                                             &si,            // Pointer to STARTUPINFO structure
                                             &pi)           // Pointer to PROCESS_INFORMATION structure
                              )
                            {
                                WaitForSingleObject(pi.hProcess, 10000);
                                DWORD n = 0;
                                GetExitCodeProcess(pi.hProcess, &n);
                                if(n == 1)
                                {
                                    cout << "Burn SUCCES..." << endl;
                                    response_str = "SUCCES";
                                }
                                else if(n == 259) // timeout
                                {
                                    cout << "Burn timeout!" << endl;
                                    response_str = "ERROR:14";
                                }
                                else
                                {
                                    cout << "Burn fail!" << endl;
                                    response_str = "ERROR:13";
                                }
                            }
                        }
                        CloseHandle(pi.hProcess);
                        CloseHandle(pi.hThread);
                    }
                    else if(file_exists(image_art)) // ART FILE Content
                    {
                        ifstream sourceFile(str_art, ios::binary);
                        ofstream destinationFile("Project.hex", ios::binary);
                        destinationFile << sourceFile.rdbuf();
                        sourceFile.close();
                        destinationFile.close();                        
                        if(Jlink_Start())
                        {
                            response_str = "ERROR:15";
                        }
                        else
                        {
                            response_str = "SUCCES";
                        }
                    }
                    else
                    {
                        cout << "Repository File not tk,mtp,hex contain!!" << endl;
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
