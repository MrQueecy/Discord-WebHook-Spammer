#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>

#pragma comment(lib, "winhttp.lib")

std::string wstringToString(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

void readResponse(HINTERNET hRequest) {
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;
    BOOL  bResults = FALSE;

    if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
        std::cerr << "WinHttpQueryDataAvailable failed: " << GetLastError() << std::endl;
        return;
    }

    pszOutBuffer = new char[dwSize + 1];
    if (!pszOutBuffer) {
        std::cerr << "Out of memory" << std::endl;
        return;
    }

    ZeroMemory(pszOutBuffer, dwSize + 1);
    if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
        std::cerr << "WinHttpReadData failed: " << GetLastError() << std::endl;
    }
    else {
        std::cout << "Response received:\n" << pszOutBuffer << std::endl;
    }

    delete[] pszOutBuffer;
}

void checkStatusCode(HINTERNET hRequest) {
    DWORD statusCode = 0;
    DWORD size = sizeof(statusCode);

    if (WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode,
        &size,
        NULL)) {
        std::cout << "Status Code: " << statusCode << std::endl;
        if (statusCode != 204) { 
            std::cerr << "Unexpected status code received." << std::endl;
        }
    }
    else {
        std::cerr << "WinHttpQueryHeaders failed: " << GetLastError() << std::endl;
    }
}

void sendMessageToDiscord(const std::wstring& webhookUrl, const std::wstring& message) {

    std::wstring host = L"discord.com";
    std::wstring path = L"/api/webhooks/your_webhook_id";

    std::string utf8Message = wstringToString(message);
    std::string jsonPayload = "{\"content\": \"" + utf8Message + "\"}";

    HINTERNET hSession = WinHttpOpen(L"A WinHTTP Example Program/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!hSession) {
        std::cerr << "WinHttpOpen failed: " << GetLastError() << std::endl;
        return;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        std::cerr << "WinHttpConnect failed: " << GetLastError() << std::endl;
        WinHttpCloseHandle(hSession);
        return;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(),
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        std::cerr << "WinHttpOpenRequest failed: " << GetLastError() << std::endl;
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return;
    }

    LPCWSTR headers = L"Content-Type: application/json\r\n";
    if (!WinHttpAddRequestHeaders(hRequest, headers, -1L, WINHTTP_ADDREQ_FLAG_ADD)) {
        std::cerr << "WinHttpAddRequestHeaders failed: " << GetLastError() << std::endl;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return;
    }

    if (!WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        (LPVOID)jsonPayload.c_str(), jsonPayload.length(),
        jsonPayload.length(),
        0)) {
        std::cerr << "WinHttpSendRequest failed: " << GetLastError() << std::endl;
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        std::cerr << "WinHttpReceiveResponse failed: " << GetLastError() << std::endl;
    }
    else {
        checkStatusCode(hRequest);
        readResponse(hRequest); 
    }

    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
}

int main() {
    std::wstring webhookUrl = L"https://discord.com/api/webhooks/your_webhook_id";
    std::wstring message = L"your webhook message";

    while (true) {
        sendMessageToDiscord(webhookUrl, message);
        Sleep(100);
    }

    return 0;
}
