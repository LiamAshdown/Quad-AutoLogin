#include "AutoLogin.h"
namespace pt = boost::property_tree;
using namespace cv;

bool close_main_thread = false;

AutoLogin::AutoLogin()
{
    WoWnd = new HWND;
    WoWMain = new HDC;
    WoWClientPosition = new RECT;
    blacklistStrikes = 0;
    // OpenCV Declaration
    cv::ocl::setUseOpenCL(false);
}

AutoLogin::~AutoLogin()
{
}

void AutoLogin::BootUp()
{
    // Declare Windows
    auto FindWoWWindow = ([]() -> HWND
    {
        std::string gameWindow = "World of Warcraft";
        HWND appWND = FindWindowA(0, gameWindow.c_str());
        return appWND;
    });

    *WoWnd = FindWoWWindow();
    if (*WoWnd)
    {
        std::cout << "Located World of Warcraft successfully!" << std::endl;
        GetWindowRect(*WoWnd, WoWClientPosition);
        *WoWMain = GetDC(HWND_DESKTOP);

        // Set WoW as our focus window
        SetFocus(*WoWnd);

        // Read XML and Check if images exist
        if (LoginStruct.LoadFile("AutoLogin.xml") && CheckImagesExist())
        {
            DBManager Database(LoginStruct.DBUser.c_str(), LoginStruct.DBPass.c_str(), LoginStruct.DBPort,
                LoginStruct.DBHost.c_str(), LoginStruct.DBName.c_str());

            if (Database.ConnectToDB())
            {
                system("cls");

                // Pass container which holds the user/pass into Autologin container
                userContainer = Database.GetUserContainer();

                MainMenu(Database);
            }
        }
        else
        {
            std::cout << "[AUTOLOGIN]: Issues loading up required data! Closing!" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            exit(-1);
        }
    }
    else
    {
        if (LoginStruct.LoadFile("AutoLogin.xml"))
        {
            std::cout << "Couldn't locate World of Warcraft! Booting up WoW client" << std::endl;
            OpenWoWClient();
            do
            {

            } while (!FindWoWWindow());
            BootUp();
        }
    }
}

void AutoLogin::MainMenu(DBManager& db)
{
    bool loop = true;
    do
    {
        int choice;
        system("cls");

        std::cout << "1. Start AutoLog Bot" << "\n";
        std::cout << "2. Exit Program" << "\n";
        std::cout << "Input: ";
        std::cin >> choice;
        std::cin.ignore(INT_MAX, '\n');
        switch (choice)
        {
        case 1:
        {
            std::cout << "\n" << std::endl;
            std::cout << "Please Enter server name: ";
            std::getline(std::cin, Server.serverName);

            // Check one more time incase user done something stupid
            auto FindWoWWindow = ([]() -> HWND
            {
                const char* WoWWindow = "World of Warcraft";
                HWND appWND = FindWindowA(0, WoWWindow);
                return appWND;
            });

            if (FindWoWWindow())
            {
                std::cout << "Booting AutoLogin..." << std::endl;

                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                MainLoop(db);
            }

            loop = false;
        }
        break;

        case 2:
        {
            system("cls");
            std::cout << "Exiting program..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            exit(-1);
        }
        break;
        default:
            std::cout << "Incorrect key! Try again!" << std::endl;
            break;
        }
    } while (loop);
}

void AutoLogin::MainLoop(DBManager& db)
{
    system("cls");
    std::thread controlThread(&AutoLogin::UserControls, this);
    controlThread.detach();

    for (std::vector<std::pair<std::string, std::string>>::const_iterator itr = userContainer.begin();
        itr != userContainer.end(); itr++)
    {
        if (close_main_thread)
            return;
        accountsScanned++;
        char* username = new char[itr->first.length() + 1];
        strcpy_s(username, sizeof(itr->first), itr->first.c_str());
        char* password = new char[itr->second.length() + 1];
        strcpy_s(password, sizeof(itr->second), itr->second.c_str());

        //Before we enter username/password, check if there's a PinPad on the screen
        if (FindPinPad())
            std::cout << "[AUTOLOGIN]: Found PinPad..." << std::endl;

        // Set cursor to login section
        SetCursorPos(WoWClientPosition->left + 465, WoWClientPosition->top + 430);
        mouse_event(MOUSEEVENTF_LEFTDOWN, WoWClientPosition->left + 465, WoWClientPosition->top + 430, 0, 0);

        // Clear anything inside username seciton
        SendCtrlAKey();

        // Enter Username and Password
        GenerateKey(username);
        SendInputKey(VK_TAB);

        GenerateKey(password);
        SendInputKey(VK_RETURN);

        // Set thread to sleep for 80ms so we don't do it too fast?
        std::this_thread::sleep_for(std::chrono::milliseconds(800));

        std::clock_t loopTimer = clock();
        double duration;
        bool exitLoop = false;
        do
        {

            if (blacklistStrikes > 3)
            {
                std::cout << "[AUTOLOGIN]: IP has been blacklisted... processing to switch IP and rebooting client!" << std::endl;

            }

            duration = (std::clock() / loopTimer) / (double)CLOCKS_PER_SEC;
            if (duration >= 15)
            {
                std::cout << "[AUTOLOGIN]: We've been inactive for 15 seconds! Resetting back to login screen!" << std::endl;
                SendInputKey(VK_ESCAPE);
                exitLoop = true;
                break;
            }

            if (FindPinPad())
            {
                std::cout << "[AUTOLOGIN]: " << username <<  " account has pinPad" << std::endl;
                exitLoop = true;
                break;
            }
            else if (FindBlizzardLogo())
            {
                blacklistStrikes++;
                std::cout << "[AUTOLOGIN]: " << username << " account has been banned... strike " << blacklistStrikes << std::endl;
                exitLoop = true;
                SendInputKey(VK_RETURN);
                break;
            }
            else if (FindTempBanned())
            {
                std::cout << "[AUTOLOGIN]: "  << username << " account is temporary banned" << std::endl;
                exitLoop = true;
                SendInputKey(VK_RETURN);
                break;
            }
            else if (FindBanned())
            {
                std::cout << "[AUTOLOGIN]: " << username << " account is banned" << std::endl;
                exitLoop = true;
                SendInputKey(VK_RETURN);
                break;
            }
            else if (FindRealmList())
            {
                SendInputKey(VK_ESCAPE);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            else if (FindSuggestRealm())
            {
                db.InsertAccount(username, password, Server.serverName.c_str());
                SendInputKey(VK_ESCAPE);
                exitLoop = true;
                break;
            }
            else if (FindEnterWorld())
            {
                db.InsertAccount(username, password, Server.serverName.c_str());
                SendInputKey(VK_ESCAPE);
                exitLoop = true;
                break;
            }
            else if (FindWorldGrey())
            {
                //db.InsertAccount(username, password, Server.serverName.c_str());
                SendInputKey(VK_ESCAPE);
                exitLoop = true;
                break;
            }
            else if (FindSessionExpired())
            {
                db.InsertAccount(username, password, Server.serverName.c_str());
                SendInputKey(VK_RETURN);
                exitLoop = true;
                break;
            }

        } while (!exitLoop);
    }
}

bool AutoLoginStruct::LoadFile(const std::string & fileName)
{
    try
    {
        pt::ptree tree;
        pt::read_xml(fileName, tree);

        DBUser = tree.get<std::string>("AutoLogin.user");
        DBPass = tree.get<std::string>("AutoLogin.password");
        DBPort = std::stoi(tree.get<std::string>("AutoLogin.port"));
        DBHost = tree.get<std::string>("AutoLogin.ip");
        DBName = tree.get<std::string>("AutoLogin.db");
        ClientDir = tree.get<std::string>("AutoLogin.dir");
        return true;
    }
    catch (...)
    {
        system("cls");
        std::cout << "Couldn't read XML file" << std::endl;
        return false;
    }

}

bool AutoLogin::CheckImagesExist()
{
    for (int i = 0; i < 7; i++)
    {
        std::ifstream ifile(fileNames[i]);

        if ((bool)ifile == false)
        {
            std::cout << "[AUTOLOGIN]: Cannot find filename " << fileNames[i] << " Closing!" << std::endl;
            return false;
        }
    }

    return true;
}

void AutoLogin::UserControls()
{
    while (true)
    {
        if (GetKeyState(VK_DELETE))
        {
            close_main_thread = true;
            std::cout << "Program stopped" << std::endl;
            std::cout << "[SYSTEM]: Scanned " << accountsScanned << " accounts" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            std::cout << "Exiting!" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            break;
        }

        if (GetKeyState(VK_DOWN))
        {
            system("cls");
            std::cout << "[SYSTEM]: Scanned " << accountsScanned << " accounts" << std::endl;
            SendInputKey(VK_DOWN);
        }
    }
}

bool AutoLogin::FindPinPad()
{
    TakeScreenShot("FindPad.bmp", false, false);
    Mat img; Mat templ; Mat mask; Mat result;
    img = cv::imread("FindPad.bmp");
    templ = cv::imread("PinPad.PNG");
    float threshold = 0.9;
    cv::ocl::setUseOpenCL(false);
    if (img.data)
    {
        Mat img_display;
        img.copyTo(img_display);
        int result_cols = img.cols - templ.cols + 1;
        int result_rows = img.rows - templ.rows + 1;

        result.create(result_rows, result_cols, CV_32FC1);

        matchTemplate(img, templ, result, CV_TM_CCORR_NORMED);
        double minVal; double maxVal; Point minLoc; Point maxLoc;
        Point matchLoc;

        minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

        matchLoc = maxLoc;

        int cursX = matchLoc.x + templ.cols;
        int cursY = (matchLoc.y + templ.rows) + 15;
        int newX = WoWClientPosition->left + cursX;
        int newY = WoWClientPosition->top + cursY;
        if (maxVal >= threshold)
        {
            if (newX >= 2430 && newY >= 897)
                return false;

            // What?
            for (int i = 0; i < 10; i++)
            {
                SetCursorPos(newX, newY);
                mouse_event(MOUSEEVENTF_LEFTDOWN, newX, newY, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, newX, newY, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTDOWN, newX, newY, 0, 0);
            }
            return true;
        }
    }
    return false;
}

void AutoLogin::TakeScreenShot(const char * BmpName, bool logo, bool emptyChar)
{
    std::string gameWindow = "World of Warcraft";
    HWND DesktopHwnd = FindWindow(0, (LPCWSTR)gameWindow.c_str());
    RECT DesktopParams;
    HDC hdcSource = GetDC(DesktopHwnd);
    HDC hdcMemory = CreateCompatibleDC(hdcSource);
    GetWindowRect(DesktopHwnd, &DesktopParams);
    DWORD Width = DesktopParams.right - DesktopParams.left;
    DWORD Height = DesktopParams.bottom - DesktopParams.top;
    if (logo)
    {
        Width = DesktopParams.right - DesktopParams.left - 100;
        Height = DesktopParams.bottom - DesktopParams.top - 300;
    }
    else
    {
        Width = DesktopParams.right - DesktopParams.left;
        Height = DesktopParams.bottom - DesktopParams.top;
    }

    int capX = GetDeviceCaps(hdcSource, HORZRES);
    int capY = GetDeviceCaps(hdcSource, VERTRES);

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcSource, Width, Height);
    HBITMAP hBitmapOld = (HBITMAP)SelectObject(hdcMemory, hBitmap);

    if (emptyChar)
    {
        BitBlt(hdcMemory, -750, 600, Width, Height, hdcSource, 0, 0, SRCCOPY);
    }
    else
        BitBlt(hdcMemory, 0, 0, Width, Height, hdcSource, 0, 0, SRCCOPY);

    hBitmap = (HBITMAP)SelectObject(hdcMemory, hBitmapOld);
    LPCSTR name = BmpName;
    HPALETTE hpal = NULL;
    SaveBitmap(name, hBitmap, hpal, BmpName);
    DeleteDC(hdcSource);
    DeleteDC(hdcMemory);
    return;
}

void AutoLogin::SaveBitmap(LPCSTR filename, HBITMAP bmp, HPALETTE pal, const char * fileName)
{
    bool result = false;
    PICTDESC pd;

    pd.cbSizeofstruct = sizeof(PICTDESC);
    pd.picType = PICTYPE_BITMAP;
    pd.bmp.hbitmap = bmp;
    pd.bmp.hpal = pal;

    LPPICTURE picture;
    HRESULT res = OleCreatePictureIndirect(&pd, IID_IPicture, false,
        reinterpret_cast<void**>(&picture));

    if (!SUCCEEDED(res))
    {
        std::cout << "Error1!" << std::endl;
        return;
    }
    LPSTREAM stream;
    res = CreateStreamOnHGlobal(0, true, &stream);

    if (!SUCCEEDED(res))
    {
        std::cout << "Error2" << std::endl;
        picture->Release();
        return;
    }

    LONG bytes_streamed;
    res = picture->SaveAsFile(stream, true, &bytes_streamed);

    HANDLE file = CreateFile((LPCWSTR)filename, GENERIC_WRITE, FILE_SHARE_READ, 0,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    // Due to Microsoft error for some reason we lose the write to create a new file.
    // We will instead open the 'existing' file and re-write it
    if (!SUCCEEDED(res) || file == INVALID_HANDLE_VALUE)
    {
        file = CreateFile((LPCWSTR)filename, GENERIC_WRITE, FILE_SHARE_READ, 0,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    }

    HGLOBAL mem = 0;
    GetHGlobalFromStream(stream, &mem);
    LPVOID data = GlobalLock(mem);

    DWORD bytes_written;

    result = !!WriteFile(file, data, bytes_streamed, &bytes_written, 0);
    result &= (bytes_written == static_cast<DWORD>(bytes_streamed));

    GlobalUnlock(mem);
    CloseHandle(file);

    stream->Release();
    picture->Release();
    return;
}

void AutoLogin::SendCtrlAKey()
{
    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    ip.ki.wVk = VK_CONTROL;
    ip.ki.dwFlags = 0; // 0 for key press
    SendInput(1, &ip, sizeof(INPUT));

    ip.ki.wVk = 'A';
    ip.ki.dwFlags = 0; // 0 for key press
    SendInput(1, &ip, sizeof(INPUT));

    // Release the "V" key
    ip.ki.wVk = 'A';
    ip.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));

    // Release the "Ctrl" key
    ip.ki.wVk = VK_CONTROL;
    ip.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));
    return;
}

void AutoLogin::GenerateKey(char* asci)
{
    for (int i = 0; i < strlen(asci); i++)
    {
        SendInputKey((UCHAR)VkKeyScan(asci[i]));
    }
}

void AutoLogin::SendInputKey(BYTE key)
{
    INPUT Input;
    ZeroMemory(&Input, sizeof(Input));
    Input.type = INPUT_KEYBOARD;
    Input.ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
    Input.ki.wVk = key;
    SendInput(1, &Input, sizeof(INPUT));
    return;
}

void AutoLogin::SendInputKey(int num)
{
    INPUT ip;

    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0; // hardware scan code for key
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;
    ip.ki.wVk = num;
    ip.ki.dwFlags = 0;
    SendInput(1, &ip, sizeof(INPUT));
    ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
    SendInput(1, &ip, sizeof(INPUT));
}

std::wstring StringToLPCWSTR(const std::string& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}

void AutoLogin::OpenWoWClient()
{
    std::wstring stemp = StringToLPCWSTR(LoginStruct.ClientDir.c_str());
    LPCWSTR result = stemp.c_str();
    //ShellExecute(NULL, L"open", result, NULL, NULL, SW_SHOW);
}

// Find certain images
bool AutoLogin::FindBlizzardLogo()
{
    TakeScreenShot("FindBlizzardLogo.bmp", true, false);
    Mat img; Mat templ; Mat mask; Mat result; Mat templ1;
    img = cv::imread("FindBlizzardLogo.bmp");
    templ = cv::imread("BlizzardLogo.PNG");
    templ1 = cv::imread("Banned.PNG");
    float threshold = 0.89;
    cv::ocl::setUseOpenCL(false);
    if (img.data)
    {
        Mat img_display;
        img.copyTo(img_display);
        int result_cols = img.cols - templ.cols + 1;
        int result_rows = img.rows - templ.rows + 1;

        result.create(result_rows, result_cols, CV_32FC1);

        matchTemplate(img, templ, result, CV_TM_CCORR_NORMED);
        double minVal; double maxVal; Point minLoc; Point maxLoc;
        Point matchLoc;

        minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

        matchLoc = maxLoc;

        int cursX = matchLoc.x + templ.cols;
        int cursY = (matchLoc.y + templ.rows) + 15;
        int newX = WoWClientPosition->left + cursX;
        int newY = WoWClientPosition->top + cursY;
        if (maxVal >= threshold)
        {
            if (newX >= 2430 && newY >= 897)
                return false;
            return true;
        }
    }
    return false;
}

bool AutoLogin::FindLoggingServer()
{
    TakeScreenShot("FindLoggingServer.bmp", false, false);
    Mat img; Mat templ; Mat mask; Mat result; Mat templ1;
    img = cv::imread("FindLoggingServer.bmp");
    templ = cv::imread("Logging.PNG");
    float threshold = 0.89;
    cv::ocl::setUseOpenCL(false);
    if (img.data)
    {
        Mat img_display;
        img.copyTo(img_display);
        int result_cols = img.cols - templ.cols + 1;
        int result_rows = img.rows - templ.rows + 1;

        result.create(result_rows, result_cols, CV_32FC1);

        matchTemplate(img, templ, result, CV_TM_CCORR_NORMED);
        double minVal; double maxVal; Point minLoc; Point maxLoc;
        Point matchLoc;
        minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

        matchLoc = maxLoc;

        int cursX = matchLoc.x + templ.cols;
        int cursY = (matchLoc.y + templ.rows) + 15;
        int newX = WoWClientPosition->left + cursX;
        int newY = WoWClientPosition->top + cursY;
        if (maxVal >= threshold)
        {
            if (newX >= 2430 && newY >= 897)
                return false;
            return true;
        }
    }
    return false;
}

bool AutoLogin::FindTempBanned()
{
    TakeScreenShot("FindTempBanned.bmp", false, false);
    Mat img; Mat templ; Mat mask; Mat result; Mat templ1;
    img = cv::imread("FindTempBanned.bmp");
    templ = cv::imread("TempBanned.PNG");
    float threshold = 0.89;
    cv::ocl::setUseOpenCL(false);
    if (img.data)
    {
        Mat img_display;
        img.copyTo(img_display);
        int result_cols = img.cols - templ.cols + 1;
        int result_rows = img.rows - templ.rows + 1;

        result.create(result_rows, result_cols, CV_32FC1);

        matchTemplate(img, templ, result, CV_TM_CCORR_NORMED);
        double minVal; double maxVal; Point minLoc; Point maxLoc;
        Point matchLoc;

        minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

        matchLoc = maxLoc;

        int cursX = matchLoc.x + templ.cols;
        int cursY = (matchLoc.y + templ.rows) + 15;
        int newX = WoWClientPosition->left + cursX;
        int newY = WoWClientPosition->top + cursY;
        if (maxVal >= threshold)
        {
            if (newX >= 2430 && newY >= 897)
                return false;
            return true;
        }
    }
    return false;
}

bool AutoLogin::FindBanned()
{
    TakeScreenShot("FindBanned.bmp", false, false);
    Mat img; Mat templ; Mat mask; Mat result; Mat templ1;
    img = cv::imread("FindBanned.bmp");
    templ = cv::imread("Banned.PNG");
    float threshold = 0.89;
    cv::ocl::setUseOpenCL(false);
    if (img.data)
    {
        Mat img_display;
        img.copyTo(img_display);
        int result_cols = img.cols - templ.cols + 1;
        int result_rows = img.rows - templ.rows + 1;

        result.create(result_rows, result_cols, CV_32FC1);

        matchTemplate(img, templ, result, CV_TM_CCORR_NORMED);
        double minVal; double maxVal; Point minLoc; Point maxLoc;
        Point matchLoc;

        minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

        matchLoc = maxLoc;

        int cursX = matchLoc.x + templ.cols;
        int cursY = (matchLoc.y + templ.rows) + 15;
        int newX = WoWClientPosition->left + cursX;
        int newY = WoWClientPosition->top + cursY;
        std::cout << "Threshold: " << threshold << std::endl;
        if (maxVal >= threshold)
        {
            if (newX >= 2430 && newY >= 897)
                return false;
            return true;
        }
    }
    return false;
}

bool AutoLogin::FindEnterWorld()
{
    TakeScreenShot("FindEnterWorld.bmp", false, false);
    Mat img; Mat templ; Mat mask; Mat result;
    img = cv::imread("FindEnterWorld.bmp");
    templ = cv::imread("EnterWorld.PNG");
    float threshold = 0.89;
    cv::ocl::setUseOpenCL(false);
    if (img.data)
    {
        Mat img_display;
        img.copyTo(img_display);
        int result_cols = img.cols - templ.cols + 1;
        int result_rows = img.rows - templ.rows + 1;

        result.create(result_rows, result_cols, CV_32FC1);

        matchTemplate(img, templ, result, CV_TM_CCORR_NORMED);
        double minVal; double maxVal; Point minLoc; Point maxLoc;
        Point matchLoc;

        minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

        matchLoc = maxLoc;

        int cursX = matchLoc.x + templ.cols;
        int cursY = (matchLoc.y + templ.rows) + 15;
        int newX = WoWClientPosition->left + cursX;
        int newY = WoWClientPosition->top + cursY;
        if (maxVal >= threshold)
        {
            if (newX >= 2430 && newY >= 897)
                return false;
            return true;
        }
    }
    return false;
}

bool AutoLogin::FindRealmList()
{
    TakeScreenShot("FindRealmList.bmp", false, false);
    Mat img; Mat templ; Mat mask; Mat result;
    img = cv::imread("FindRealmList.bmp");
    templ = cv::imread("RealmList.PNG");
    float threshold = 0.89;
    cv::ocl::setUseOpenCL(false);
    if (img.data)
    {
        Mat img_display;
        img.copyTo(img_display);
        int result_cols = img.cols - templ.cols + 1;
        int result_rows = img.rows - templ.rows + 1;

        result.create(result_rows, result_cols, CV_32FC1);

        matchTemplate(img, templ, result, CV_TM_CCORR_NORMED);
        double minVal; double maxVal; Point minLoc; Point maxLoc;
        Point matchLoc;

        minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

        matchLoc = maxLoc;

        int cursX = matchLoc.x + templ.cols;
        int cursY = (matchLoc.y + templ.rows) + 15;
        int newX = WoWClientPosition->left + cursX;
        int newY = WoWClientPosition->top + cursY;
        if (maxVal >= threshold)
        {
            if (newX >= 2430 && newY >= 897)
                return false;
            return true;

        }
    }
    return false;
}

bool AutoLogin::FindSuggestRealm()
{
    TakeScreenShot("FindSuggestRealm.bmp", false, false);
    Mat img; Mat templ; Mat mask; Mat result;
    img = cv::imread("FindSuggestRealm.bmp");
    templ = cv::imread("Suggest.PNG");
    float threshold = 0.89;
    cv::ocl::setUseOpenCL(false);
    if (img.data)
    {
        Mat img_display;
        img.copyTo(img_display);
        int result_cols = img.cols - templ.cols + 1;
        int result_rows = img.rows - templ.rows + 1;

        result.create(result_rows, result_cols, CV_32FC1);

        matchTemplate(img, templ, result, CV_TM_CCORR_NORMED);
        double minVal; double maxVal; Point minLoc; Point maxLoc;
        Point matchLoc;

        minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

        matchLoc = maxLoc;

        int cursX = matchLoc.x + templ.cols;
        int cursY = (matchLoc.y + templ.rows) + 15;
        std::string gameWindow = "World of Warcraft";
        int newX = WoWClientPosition->left + cursX;
        int newY = WoWClientPosition->top + cursY;
        if (maxVal >= threshold)
        {
            if (newX >= 2430 && newY >= 897)
                return false;
            return true;
        }
    }
    return false;
}

bool AutoLogin::FindSessionExpired()
{
    Mat img; Mat templ; Mat mask; Mat result;
    TakeScreenShot("FindSessionExpired.bmp", false, false);
    img = cv::imread("FindSessionExpired.bmp");
    templ = cv::imread("SessionExpired.PNG");
    float threshold = 0.89;
    cv::ocl::setUseOpenCL(false);
    if (img.data)
    {
        Mat img_display;
        img.copyTo(img_display);
        int result_cols = img.cols - templ.cols + 1;
        int result_rows = img.rows - templ.rows + 1;

        result.create(result_rows, result_cols, CV_32FC1);

        matchTemplate(img, templ, result, CV_TM_CCORR_NORMED);
        double minVal; double maxVal; Point minLoc; Point maxLoc;
        Point matchLoc;

        minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

        matchLoc = maxLoc;

        int cursX = matchLoc.x + templ.cols;
        int cursY = (matchLoc.y + templ.rows) + 15;
        int newX = WoWClientPosition->left + cursX;
        int newY = WoWClientPosition->top + cursY;
        if (maxVal >= threshold)
        {
            if (newX >= 2430 && newY >= 897)
                return false;
            return true;
        }
    }
    return false;
}

bool AutoLogin::FindWorldGrey()
{
    Mat img; Mat templ; Mat mask; Mat result;
    std::this_thread::sleep_for(std::chrono::seconds(1500));
    TakeScreenShot("FindWorldGrey.bmp", false, false);
    img = cv::imread("FindWorldGrey.bmp");

    templ = cv::imread("EnterWorldGrey.PNG");
    float threshold = 0.89;
    cv::ocl::setUseOpenCL(false);
    if (img.data)
    {
        Mat img_display;
        img.copyTo(img_display);
        int result_cols = img.cols - templ.cols + 1;
        int result_rows = img.rows - templ.rows + 1;

        result.create(result_rows, result_cols, CV_32FC1);

        matchTemplate(img, templ, result, CV_TM_CCORR_NORMED);
        double minVal; double maxVal; Point minLoc; Point maxLoc;
        Point matchLoc;

        minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

        matchLoc = maxLoc;

        int cursX = matchLoc.x + templ.cols;
        int cursY = (matchLoc.y + templ.rows) + 15;
        int newX = WoWClientPosition->left + cursX;
        int newY = WoWClientPosition->top + cursY;
        if (maxVal >= threshold)
        {
            if (newX >= 2430 && newY >= 897)
                return false;
            std::cout << "Character is empty! " << std::endl;
            return true;
        }
    }
    std::cout << "Character is not empty! " << std::endl;
    return false;
}