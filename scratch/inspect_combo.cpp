#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

int main() {
    HWND hwnd = FindWindowW(L"MainClass", L"ID Card Printer Pro");
    if (!hwnd) {
        std::cout << "Main window not found!" << std::endl;
        return 1;
    }

    HWND hCombo = FindWindowExW(hwnd, NULL, L"COMBOBOX", NULL);
    if (!hCombo) {
        std::cout << "ComboBox not found!" << std::endl;
        return 1;
    }

    int count = SendMessage(hCombo, CB_GETCOUNT, 0, 0);
    std::cout << "Paper Size Count: " << count << std::endl;

    for (int i = 0; i < count; ++i) {
        int len = SendMessage(hCombo, CB_GETLBTEXTLEN, i, 0);
        if (len != CB_ERR) {
            std::vector<wchar_t> buf(len + 1);
            SendMessage(hCombo, CB_GETLBTEXT, i, (LPARAM)buf.data());
            std::wcout << i << L": " << buf.data() << std::endl;
        }
    }

    return 0;
}
