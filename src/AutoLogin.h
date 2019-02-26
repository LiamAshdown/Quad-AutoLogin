#pragma once
#include "DBManager.h"
#include "IPSwitcher.h"
#include <Windows.h>


struct AutoLoginStruct
{
public:
    AutoLoginStruct() {}

public:
    
    std::string FileName;
    std::string DBUser;
    std::string DBPass;
    unsigned int DBPort;
    std::string DBHost;
    std::string DBName;
    std::string ClientDir;
    bool LoadFile(const std::string &fileName);
};

typedef AutoLoginStruct AutoStruct;

struct ServerStruct
{
    std::string serverName;
};

typedef ServerStruct ServerDetail;

class AutoLogin
{
public:
    AutoLogin();
    ~AutoLogin();

    void BootUp();

protected:
    void MainMenu(DBManager& db);
    void MainLoop(DBManager& db);
    void UserControls();
    void TakeScreenShot(const char * BmpName, bool logo, bool emptyChar);
    void SaveBitmap(LPCSTR filename, HBITMAP bmp, HPALETTE pal, const char * fileName);
    bool CheckImagesExist();
    void SendCtrlAKey();
    void GenerateKey(char* asci);
    void SendInputKey(BYTE key);
    void SendInputKey(int num);
    void OpenWoWClient();

// OpenCV
protected:
    bool FindPinPad();
    bool FindBlizzardLogo();
    bool FindTempBanned();
    bool FindBanned();
    bool FindEnterWorld();
    bool FindRealmList();
    bool FindSuggestRealm();
    bool FindSessionExpired();
    bool FindWorldGrey();
    bool FindLoggingServer();



protected:
    HWND* WoWnd;
    RECT* WoWClientPosition;
    HDC* WoWMain;
    AutoStruct LoginStruct;
    ServerDetail Server;
    const char* fileNames[8] =
    {
        {"PinPad.PNG"},
        {"Banned.PNG"},
        {"EnterWorld.PNG"},
        {"RealmList.PNG"},
        {"SuggestRealm.PNG"},
        {"SessionExpired.PNG"},
        {"EnterWorldGrey.PNG"},
        {"EnterWorldGrey.PNG"},
    };
    std::vector<std::pair<std::string, std::string>> userContainer;
    unsigned long int accountsScanned;
    unsigned int blacklistStrikes;
};

