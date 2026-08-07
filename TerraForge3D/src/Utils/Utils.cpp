#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "Utils/Utils.h"
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <httplib/httplib.h>
#include <iostream>
#include <initializer_list>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#elif __linux__
#include <inttypes.h>
#include <linux/limits.h> // PATH_MAX
#include <unistd.h>
#define __int64   int64_t
#define _close    close
#define _read     read
#define _lseek64  lseek64
#define _O_RDONLY O_RDONLY
#define _open     open
#define _lseeki64 lseek64
#define _lseek    lseek
#define stricmp   strcasecmp
#endif

#include "resource.h"

#ifdef TERR3D_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32 // For Windows
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#endif

namespace tf3d::utils
{

    void HashCombine64(uint64_t &hash, uint64_t value)
    {
        hash ^= value;
        hash *= 1099511628211ull;
    }

    uint64_t QuantizeFloatForHash(float value, float precision)
    {
        if (std::isnan(value))
            return 0x7ff8000000000000ull;
        if (std::isinf(value))
            return value > 0.0f
                       ? 0x7ff0000000000000ull
                       : 0xfff0000000000000ull;
        return static_cast<uint64_t>(std::llround(
            static_cast<double>(value) / static_cast<double>(precision)));
    }

    float OrderedUintToFloat(uint32_t value)
    {
        const uint32_t bits = (value & 0x80000000u) != 0u
                                  ? value ^ 0x80000000u
                                  : ~value;
        float result        = 0.0f;
        std::memcpy(&result, &bits, sizeof(result));
        return result;
    }

    float RawUintToFloat(uint32_t value)
    {
        float result = 0.0f;
        std::memcpy(&result, &value, sizeof(result));
        return result;
    }

} // namespace tf3d::utils

// SAF_Handle.cpp line:458 old line:INFILE = _open(infilename, _O_RDONLY | _O_BINARY);
#ifdef __linux__
//  INFILE = _open(infilename, _O_RDONLY);
#else
//  INFILE = _open(infilename, _O_RDONLY | _O_BINARY);
#endif

#include "Application.h"
#include "GLFW/glfw3.h"

#ifndef TERR3D_WIN32
#include <libgen.h> // dirname
#include <unistd.h> // readlink
#define MAX_PATH PATH_MAX
#else
#include <Commdlg.h>
#include <atlstr.h>
#include <shellapi.h>
#include <windows.h>
#endif
#ifdef __APPLE__
#include <libproc.h>
#endif // DEBUG

namespace tf3d::utils
{

    static std::string getExecutablePath()
    {
        char rawPathName[MAX_PATH];
#ifdef TERR3D_WIN32
        GetModuleFileNameA(NULL, rawPathName, MAX_PATH);
#elif defined(__APPLE__)
        proc_pidpath(getpid(), rawPathName, MAX_PATH);
#else
        readlink("/proc/self/exe", rawPathName, PATH_MAX);
#endif
        return std::string(rawPathName);
    }

    static std::string getExecutableDir()
    {
        std::string executablePath = getExecutablePath();
        std::string directory      = executablePath.substr(0, executablePath.find_last_of("\\/"));
        return directory;
    }

    const char HEX_MAP[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    char replace(unsigned char c)
    {
        return HEX_MAP[c & 0x0f];
    }

    std::string UChar2Hex(unsigned char c)
    {
        {
            std::string hex;
            // First four bytes
            char left = (c >> 4);
            // Second four bytes
            char right = (c & 0x0f);
            hex += replace(left);
            hex += replace(right);
            return hex;
        }
    }

#ifdef TERR3D_WIN32

    std::wstring s2ws(const std::string &s)
    {
        int len;
        int slength  = (int)s.length() + 1;
        len          = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
        wchar_t *buf = new wchar_t[len];
        MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
        std::wstring r(buf);
        delete[] buf;
        return r;
    }

#endif

    std::string ShowSaveFileDialog(std::string ext)
    {
#ifdef TERR3D_WIN32
        OPENFILENAME ofn;
        CHAR fileName[MAX_PATH];
        ZeroMemory(fileName, MAX_PATH);
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner   = NULL;
        // ofn.lpstrFilter = s2ws(ext).c_str();
        ofn.lpstrFilter = "*.*\0";
        ofn.lpstrFile   = fileName;
        ofn.nMaxFile    = MAX_PATH;
        ofn.Flags       = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        ofn.lpstrDefExt = "";
        std::string fileNameStr;

        if (GetSaveFileName(&ofn)) {
            // std::wstring ws(ofn.lpstrFile);
            std::string str = std::string(ofn.lpstrFile);
            // for (auto ch : ws) str += (char)ch;
            return str;
        }

        return std::string("");
#else
        char filename[PATH_MAX];
        FILE *f = popen("zenity --file-selection --save", "r");
        fgets(filename, PATH_MAX, f);
        pclose(f);
        filename[strcspn(filename, "\n")] = 0;
        return std::string(filename);
#endif
    }

    std::string openfilename()
    {
        return ShowOpenFileDialog(".obj");
    }

    bool DeleteFileT(std::string path)
    {
        try {
            if (std::filesystem::remove(path)) {
                TF3D_LOG_DEBUG("Deleted file '{}'", path);
            }

            else {
                TF3D_LOG_WARN("Could not delete file '{}'", path);
                return false;
            }
        }

        catch (const std::filesystem::filesystem_error &err) {
            TF3D_LOG_ERROR("Filesystem error while deleting '{}': {}", path, err.what());
            return false;
        }

        return true;
    }

    std::string ShowOpenFileDialog(std::string ext)
    {
#ifdef TERR3D_WIN32
        OPENFILENAME ofn;
        CHAR fileName[MAX_PATH];
        ZeroMemory(fileName, MAX_PATH);
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner   = NULL;
        // ofn.lpstrFilter = s2ws(ext).c_str();
        ofn.lpstrFilter = "*.*\0";
        ofn.lpstrFile   = fileName;
        ofn.nMaxFile    = MAX_PATH;
        ofn.Flags       = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        ofn.lpstrDefExt = "";
        std::string fileNameStr;

        if (GetOpenFileName(&ofn)) {
            // std::wstring ws(ofn.lpstrFile);
            //  your new String
            // std::string str(ws.begin(), ws.end());
            return std::string(ofn.lpstrFile);
        }

        return std::string("");
#else
        char filename[PATH_MAX];
        FILE *f = popen("zenity --file-selection", "r");
        fgets(filename, PATH_MAX, f);
        pclose(f);
        filename[strcspn(filename, "\n")] = 0;
        return std::string(filename);
#endif
    }

    std::string ReadShaderSourceFile(std::string path, bool *result)
    {
        std::fstream newfile;
        newfile.open(path.c_str(), std::ios::in);
        if (newfile.is_open()) {
            std::string tp;
            std::string res = "";
            getline(newfile, res, '\0');
            newfile.close();
            *result = true;
            return res;
        } else
            *result = false;

        return std::string("");
    }

    bool WriteShaderSourceFile(const std::string &path, const std::string &content)
    {
        std::ofstream file;
        file.open(path);
        if (!file.is_open())
            return false;
        file << content;
        file.close();
        return true;
    }

    std::string GetExecutablePath()
    {
        return getExecutablePath();
    }

    std::string GetExecutableDir()
    {
        return getExecutableDir();
    }

    std::string GenerateId(uint32_t length)
    {
        std::string id;

        for (uint32_t i = 0; i < length; i++) {
            id += std::to_string(rand() % 9);
        }

        return id;
    }

    std::string FetchURL(std::string baseURL, std::string path, std::string token)
    {
        httplib::Client cli(baseURL);
        httplib::Headers headers;
        if (!token.empty())
            headers.emplace("Authorization", "Bearer " + token);
        auto res = cli.Get(path.c_str(), headers);
        cli.set_read_timeout(10);

        if (res.error() == httplib::Error::Success) {
            try {
                return res->body;
            }

            catch (...) {
            }
        } else {
            TF3D_LOG_ERROR("HTTP request failed for '{}{}': {}", baseURL, path, httplib::to_string(res.error()));
        }

        return "";
    }

    // From : https://stackoverflow.com/a/10467633/14911094
    // Get current date/time, format is YYYY-MM-DD.HH:mm:ss
    std::string GetTimeStamp()
    {
        time_t now = time(0);
        struct tm tstruct;
        char buf[80];
        tstruct = *localtime(&now);
        // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
        // for more information about date/time format
        strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

        return buf;
    }

    char *UChar2Char(unsigned char *data, int length)
    {
        char *odata = new char[length];

        for (int i = 0; i < length; i++) {
            odata[i] = (char)data[i];
        }

        return odata;
    }

    bool FileExists(std::string path, bool writeAccess)
    {
#ifdef TERR3D_WIN32

        if ((_access(path.c_str(), 0)) != -1) {
            if (!writeAccess) {
                return true;
            }

            if ((_access(path.c_str(), 2)) != -1) {
                return true;
            }
        }

        return false;
#else
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
#endif
    }

    bool PathExist(const std::string &s)
    {
        struct stat buffer;
        return (stat(s.c_str(), &buffer) == 0);
    }

#ifdef TERR3D_WIN32
#include <wininet.h>
#pragma comment(lib, "Wininet.lib")
#endif

    bool IsNetWorkConnected()
    {
#ifdef TERR3D_WIN32
        bool bConnect = InternetCheckConnectionW(L"http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0);
        return bConnect;
#else
        // Return true if internet connection is alive else false
        return true;
#endif
    }

    std::string FormatMemoryToString(uint64_t size)
    {
        std::string result = "";
        if (size <= 1000)
            return std::to_string(size) + " B";
        else if (size <= 1000000)
            return std::to_string(size / 1000.0) + " KB";
        else if (size <= 1000000000)
            return std::to_string(size / (1000000.0)) + " MB";
        else if (size <= 1000000000000)
            return std::to_string(size / (1000000000.0)) + " GB";
        else if (size <= 1000000000000000)
            return std::to_string(size / (1000000000000.0)) + " TB";
        return std::to_string(size) + " B";
    }

    std::string LowercaseFilterText(const std::string &value)
    {
        std::string result;
        result.reserve(value.size());
        for (const char character : value)
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
        return result;
    }

    bool FuzzyFilterMatch(const std::string &query, const std::string &candidate)
    {
        const std::string normalizedQuery = LowercaseFilterText(query);
        if (normalizedQuery.empty())
            return true;

        const std::string normalizedCandidate = LowercaseFilterText(candidate);
        size_t candidateIndex                 = 0;
        for (const char queryCharacter : normalizedQuery) {
            candidateIndex = normalizedCandidate.find(queryCharacter, candidateIndex);
            if (candidateIndex == std::string::npos)
                return false;
            candidateIndex++;
        }
        return true;
    }

    float UpdateLayerWithUpdateMethod(float origv, float newv, int method)
    {
        if (method == 0)
            return newv;
        else if (method == 1)
            return origv + newv;
        else if (method == 2)
            return origv - newv;
        else if (method == 3)
            return origv * newv;
        return origv;
    }

    char *ReadBinaryFile(std::string path, int *fSize, int32_t sizeToLoad)
    {
        std::ifstream in(path, std::ifstream::ate | std::ifstream::binary);
        int size = (int)in.tellg();
        in.close();

        if (sizeToLoad > 0) {
            size = size < sizeToLoad ? size : sizeToLoad;
        }

        std::ifstream f1(path, std::fstream::binary);

        if (size < 1) {
            size = 1024;
        }

        char *buffer = new char[size];
        f1.read(buffer, size);
        f1.close();
        *fSize = size;
        return buffer;
    }

    char *ReadBinaryFile(std::string path, int32_t sizeToLoad)
    {
        int size;
        return ReadBinaryFile(path, &size, sizeToLoad);
    }

    Hash MD5File(std::string path)
    {
        int size            = 1;
        unsigned char *data = (unsigned char *)ReadBinaryFile(path, &size);
        unsigned char hash[MD5_DIGEST_LENGTH];
        MD5(data, size, hash);
        Hash resultHash(hash, MD5_DIGEST_LENGTH);
        delete[] data;
        return resultHash;
    }

    void DownloadFile(std::string baseURL, std::string urlPath, std::string path, int size)
    {
        try {
            std::ofstream outfile;
            httplib::Client cli(baseURL);
            int done = 0;
            outfile.open(path.c_str(), std::ios::binary | std::ios::out);
            TF3D_LOG_INFO("Starting download: {}{}", baseURL, urlPath);
            auto res = cli.Get(urlPath.c_str(),
                               [&](const char *data, size_t data_length) {
                                   done = done + (int)data_length;

                                   if (size > 0) {
                                       float percent = (float)done / (float)size;
                                       TF3D_LOG_DEBUG("Download progress: {}%", static_cast<int>(percent * 100));
                                   }

                                   outfile.write(data, data_length);
                                   return true;
                               });
            TF3D_LOG_INFO("Download complete: '{}'", path);
            outfile.close();
        }

        catch (...) {
        }
    }

    void SaveToFile(std::string filename, std::string content)
    {
        std::ofstream outfile;
        outfile.open(filename);
        outfile << content;
        outfile.close();
    }

#ifdef TERR3D_WIN32

    // From https://stackoverflow.com/a/20256714/14911094
    void RegSet(HKEY hkeyHive, const char *pszVar, const char *pszValue)
    {
        HKEY hkey;
        char szValueCurrent[1000];
        DWORD dwType;
        DWORD dwSize     = sizeof(szValueCurrent);
        int iRC          = RegGetValue(hkeyHive, CA2CT(pszVar), NULL, RRF_RT_ANY, &dwType, szValueCurrent, &dwSize);
        bool bDidntExist = iRC == ERROR_FILE_NOT_FOUND;

        if (iRC != ERROR_SUCCESS && !bDidntExist) {
            TF3D_LOG_ERROR("RegGetValue failed for '{}': {}", pszVar, strerror(iRC));
        }

        if (!bDidntExist) {
            if (dwType != REG_SZ) {
                TF3D_LOG_WARN("RegGetValue returned unsupported type {} for '{}'", dwType, pszVar);
            }

            if (strcmp(szValueCurrent, pszValue) == 0) {
                TF3D_LOG_DEBUG("Registry value '{}' is already '{}'; no update needed", pszVar, pszValue);
                return;
            }
        }

        DWORD dwDisposition;
        iRC = RegCreateKeyEx(hkeyHive, CA2CT(pszVar), 0, 0, 0, KEY_ALL_ACCESS, NULL, &hkey, &dwDisposition);

        if (iRC != ERROR_SUCCESS) {
            TF3D_LOG_ERROR("RegSetValue failed for '{}': {}", pszVar, strerror(iRC));
        }

        iRC = RegSetValueEx(hkey, "", 0, REG_SZ, (BYTE *)pszValue, (int)strlen(pszValue) + 1);

        if (iRC != ERROR_SUCCESS) {
            TF3D_LOG_ERROR("RegSetValue failed for '{}': {}", pszVar, strerror(iRC));
        }

        if (bDidntExist) {
            TF3D_LOG_INFO("Registry value '{}' set to '{}'", pszVar, pszValue);
        }

        else {
            TF3D_LOG_INFO("Registry value '{}' changed from '{}' to '{}'", pszVar, szValueCurrent, pszValue);
        }

        RegCloseKey(hkey);
    }

#endif

    void SetUpIcon()
    {
#ifdef TERR3D_WIN32
        HWND hwnd   = glfwGetWin32Window(base::Application::Get()->GetWindow()->GetNativeWindow());
        HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
        if (hIcon) {
            // Change both icons to the same icon handle.
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            // This will ensure that the application icon gets changed too.
            SendMessage(GetWindow(hwnd, GW_OWNER), WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(GetWindow(hwnd, GW_OWNER), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }
#endif
    }

    void AccocFileType()
    {
#ifdef TERR3D_WIN32
        RegSet(HKEY_CURRENT_USER, "Software\\Classes\\.terr3d", "TerraGen3D.TerraGen3D.1");
        // Not needed.
        RegSet(HKEY_CURRENT_USER, "Software\\Classes\\.terr3d\\Content Type", "application/json");
        RegSet(HKEY_CURRENT_USER, "Software\\Classes\\.terr3d\\PerceivedType", "json");
        // Not needed, but may be be a way to have wordpad show up on the default list.
        RegSet(HKEY_CURRENT_USER, "Software\\Classes\\.terr3d\\OpenWithProgIds\\TerraGen3D.TerraGen3D.1", "");
        RegSet(HKEY_CURRENT_USER, "Software\\Classes\\TerraGen3D.TerraGen3D.1", "TerraGen3D");
        RegSet(HKEY_CURRENT_USER, "Software\\Classes\\TerraGen3D.TerraGen3D.1\\Shell\\Open\\Command", (getExecutablePath() + " %1").c_str());
#endif //  TERR3D_WIN32
    }

    void MkDir(std::string path)
    {
        if (!PathExist(path)) {
#ifdef TERR3D_WIN32
            system((std::string("mkdir \"") + path + "\"").c_str());
#else
            system((std::string("mkdir -p \"") + path + "\"").c_str());
#endif
        }
    }

#ifndef TERR3D_WIN32
    int get_file_size(char *source)
    {
        FILE *fichier = fopen(source, "rb");
        fseek(fichier, 0, SEEK_END);
        int size = ftell(fichier);
        fseek(fichier, 0, SEEK_SET);
        fclose(fichier);
        return size;
    }
#endif

    void CopyFileData(std::string source, std::string destination)
    {
#ifdef TERR3D_WIN32
        // CopyFileW(CString(source.c_str()), CString(destination.c_str()), false);
        CopyFile(source.c_str(), destination.c_str(), false);
#else
        int srcsize = get_file_size(source.data());
        char *data  = (char *)malloc(srcsize);
        int fsource = open(source.c_str(), O_RDONLY);
        int fdest   = open(destination.c_str(), O_WRONLY | O_CREAT, 0777);
        read(fsource, data, srcsize);
        write(fdest, data, srcsize);
        close(fsource);
        close(fdest);
        free(data);
#endif
    }

    bool IsKeyDown(int key)
    {
        return glfwGetKey(base::Application::Get()->GetWindow()->GetNativeWindow(), key) == GLFW_PRESS;
    }

    bool IsMouseButtonDown(int button)
    {
        return glfwGetMouseButton(base::Application::Get()->GetWindow()->GetNativeWindow(), button);
    }

    void ShowMessageBox(std::string message, std::string title)
    {
#ifdef TERR3D_WIN32
        // MessageBox(NULL, s2ws(message).c_str(), s2ws(title).c_str(), 0);
        MessageBox(NULL, message.c_str(), title.c_str(), 0);
#else
        system(("zenity --info --text=" + message + " --title=" + title + "").c_str());
#endif
    }

    void ToggleSystemConsole()
    {
        static bool state = false;
        state             = !state;
#ifdef TERR3D_WIN32
        ShowWindow(GetConsoleWindow(), state ? SW_SHOW : SW_HIDE);
#else
        TF3D_LOG_WARN("Console toggling is not supported on Linux");
#endif
    }

    std::string ColorConvertToHexString(float r, float g, float b, float a)
    {
        // Multiply each color component by 255 and round
        int ir = static_cast<int>(r * 255.0f + 0.5f);
        int ig = static_cast<int>(g * 255.0f + 0.5f);
        int ib = static_cast<int>(b * 255.0f + 0.5f);
        int ia = static_cast<int>(a * 255.0f + 0.5f);

        // Convert each component to a hex string
        std::stringstream stream;
        stream << std::setfill('0') << std::setw(2) << std::hex << ir;
        stream << std::setfill('0') << std::setw(2) << std::hex << ig;
        stream << std::setfill('0') << std::setw(2) << std::hex << ib;
        stream << std::setfill('0') << std::setw(2) << std::hex << ia;

        return stream.str();
    }

    void OpenURL(std::string url)
    {
#ifdef TERR3D_WIN32
        std::string op = std::string("start ").append(url);
        system(op.c_str());
#else
        std::string op = std::string("xdg-open ").append(url);
        system(op.c_str());
#endif //  TERR3D_WIN32
    }

    namespace
    {
        std::string EnvironmentValue(const char *name)
        {
            const char *value = std::getenv(name);
            return value != nullptr ? std::string(value) : std::string();
        }

        bool IsRegularFile(const std::filesystem::path &path)
        {
            std::error_code error;
            return std::filesystem::is_regular_file(path, error);
        }

    } // namespace

    std::string FindExecutableOnPath(std::initializer_list<const char *> names)
    {
#ifdef TERR3D_WIN32
        std::array<char, 32768> resolvedPath{};
        for (const char *name : names) {
            const DWORD length = SearchPathA(nullptr,
                                             name,
                                             nullptr,
                                             static_cast<DWORD>(resolvedPath.size()),
                                             resolvedPath.data(),
                                             nullptr);
            if (length > 0 && length < resolvedPath.size())
                return std::string(resolvedPath.data(), length);
        }
#else
        const std::string pathValue = EnvironmentValue("PATH");
        std::size_t pathStart       = 0;
        while (pathStart <= pathValue.size()) {
            const std::size_t pathEnd   = pathValue.find(':', pathStart);
            const std::string directory = pathValue.substr(
                pathStart,
                pathEnd == std::string::npos ? std::string::npos : pathEnd - pathStart);

            if (!directory.empty()) {
                for (const char *name : names) {
                    const std::filesystem::path candidate = std::filesystem::path(directory) / name;
                    if (IsRegularFile(candidate))
                        return candidate.string();
                }
            }

            if (pathEnd == std::string::npos)
                break;
            pathStart = pathEnd + 1;
        }
#endif
        return {};
    }


    void OnBeforeImGuiRender()
    {
        static bool dockspaceOpen                 = true;
        static bool opt_fullscreen_persistant     = true;
        bool opt_fullscreen                       = opt_fullscreen_persistant;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        ImGuiWindowFlags window_flags             = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

        if (opt_fullscreen) {
            ImGuiViewport *viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.1f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) {
            window_flags |= ImGuiWindowFlags_NoBackground;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen) {
            ImGui::PopStyleVar(2);
        }

        // DockSpace
        ImGuiIO &io           = ImGui::GetIO();
        ImGuiStyle &style     = ImGui::GetStyle();
        float minWinSizeX     = style.WindowMinSize.x;
        style.WindowMinSize.x = 370.0f;

        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        style.WindowMinSize.x = minWinSizeX;
    }

    void OnImGuiRenderEnd()
    {
        ImGui::End();
    }

} // namespace tf3d::utils
