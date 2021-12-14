#pragma once



#include <Mesh.h>
#include <string>



std::string UChar2Hex(unsigned char c);

struct Hash {

	Hash() {

	}

	Hash(unsigned char* d, int l) {
		length = l;
		data = new unsigned char[length];
		memcpy_s(data, length, d, l);
	}

	~Hash() {
		//delete[] data;
	}

	std::string ToString() {
		std::string hash;
		for (int i = 0; i < length; i++) 
		{
			
			hash += UChar2Hex(data[i]);
		}
		return hash;
	}

	int length = 0;
	unsigned char* data;
};


#define MAX(x, y) (x > y ? x : y)

std::string ShowSaveFileDialog(std::string ext = ".terr3d");

std::string openfilename();

std::string ShowOpenFileDialog(std::string ext = "*.glsl");

std::string ReadShaderSourceFile(std::string path, bool* result);

std::string GetExecutablePath();

std::string GetExecutableDir();

std::string GenerateId(uint32_t length);

std::string FetchURL(std::string baseURL, std::string path);

char* UChar2Char(unsigned char* data, int length);

bool FileExists(std::string path, bool writeAccess = false);

bool PathExist(const std::string& s);

bool IsNetWorkConnected();

char* ReadBinaryFile(std::string path, int* size, uint32_t sizeToLoad = -1);

char* ReadBinaryFile(std::string path, uint32_t sizeToLoad = -1);

Hash MD5File(std::string path);

void DownloadFile(std::string baseURL, std::string urlPath, std::string path, int size = -1);

void SaveToFile(std::string filename, std::string content = "");

void Log(const char* log);

void Log(std::string log);

#ifdef TERR3D_WIN32
#include <windows.h>
void RegSet(HKEY hkeyHive, const char* pszVar, const char* pszValue);
#endif

void AccocFileType();

void MkDir(std::string path);

void CopyFileData(std::string source, std::string destination);

bool IsKeyDown(int key);


// KEY DEFINES
/* The unknown key */
#define TERR3D_KEY_UNKNOWN            -1

/* Printable keys */
#define TERR3D_KEY_SPACE              32
#define TERR3D_KEY_APOSTROPHE         39  /* ' */
#define TERR3D_KEY_COMMA              44  /* , */
#define TERR3D_KEY_MINUS              45  /* - */
#define TERR3D_KEY_PERIOD             46  /* . */
#define TERR3D_KEY_SLASH              47  /* / */
#define TERR3D_KEY_0                  48
#define TERR3D_KEY_1                  49
#define TERR3D_KEY_2                  50
#define TERR3D_KEY_3                  51
#define TERR3D_KEY_4                  52
#define TERR3D_KEY_5                  53
#define TERR3D_KEY_6                  54
#define TERR3D_KEY_7                  55
#define TERR3D_KEY_8                  56
#define TERR3D_KEY_9                  57
#define TERR3D_KEY_SEMICOLON          59  /* ; */
#define TERR3D_KEY_EQUAL              61  /* = */
#define TERR3D_KEY_A                  65
#define TERR3D_KEY_B                  66
#define TERR3D_KEY_C                  67
#define TERR3D_KEY_D                  68
#define TERR3D_KEY_E                  69
#define TERR3D_KEY_F                  70
#define TERR3D_KEY_G                  71
#define TERR3D_KEY_H                  72
#define TERR3D_KEY_I                  73
#define TERR3D_KEY_J                  74
#define TERR3D_KEY_K                  75
#define TERR3D_KEY_L                  76
#define TERR3D_KEY_M                  77
#define TERR3D_KEY_N                  78
#define TERR3D_KEY_O                  79
#define TERR3D_KEY_P                  80
#define TERR3D_KEY_Q                  81
#define TERR3D_KEY_R                  82
#define TERR3D_KEY_S                  83
#define TERR3D_KEY_T                  84
#define TERR3D_KEY_U                  85
#define TERR3D_KEY_V                  86
#define TERR3D_KEY_W                  87
#define TERR3D_KEY_X                  88
#define TERR3D_KEY_Y                  89
#define TERR3D_KEY_Z                  90
#define TERR3D_KEY_LEFT_BRACKET       91  /* [ */
#define TERR3D_KEY_BACKSLASH          92  /* \ */
#define TERR3D_KEY_RIGHT_BRACKET      93  /* ] */
#define TERR3D_KEY_GRAVE_ACCENT       96  /* ` */
#define TERR3D_KEY_WORLD_1            161 /* non-US #1 */
#define TERR3D_KEY_WORLD_2            162 /* non-US #2 */

/* Function keys */
#define TERR3D_KEY_ESCAPE             256
#define TERR3D_KEY_ENTER              257
#define TERR3D_KEY_TAB                258
#define TERR3D_KEY_BACKSPACE          259
#define TERR3D_KEY_INSERT             260
#define TERR3D_KEY_DELETE             261
#define TERR3D_KEY_RIGHT              262
#define TERR3D_KEY_LEFT               263
#define TERR3D_KEY_DOWN               264
#define TERR3D_KEY_UP                 265
#define TERR3D_KEY_PAGE_UP            266
#define TERR3D_KEY_PAGE_DOWN          267
#define TERR3D_KEY_HOME               268
#define TERR3D_KEY_END                269
#define TERR3D_KEY_CAPS_LOCK          280
#define TERR3D_KEY_SCROLL_LOCK        281
#define TERR3D_KEY_NUM_LOCK           282
#define TERR3D_KEY_PRINT_SCREEN       283
#define TERR3D_KEY_PAUSE              284
#define TERR3D_KEY_F1                 290
#define TERR3D_KEY_F2                 291
#define TERR3D_KEY_F3                 292
#define TERR3D_KEY_F4                 293
#define TERR3D_KEY_F5                 294
#define TERR3D_KEY_F6                 295
#define TERR3D_KEY_F7                 296
#define TERR3D_KEY_F8                 297
#define TERR3D_KEY_F9                 298
#define TERR3D_KEY_F10                299
#define TERR3D_KEY_F11                300
#define TERR3D_KEY_F12                301
#define TERR3D_KEY_F13                302
#define TERR3D_KEY_F14                303
#define TERR3D_KEY_F15                304
#define TERR3D_KEY_F16                305
#define TERR3D_KEY_F17                306
#define TERR3D_KEY_F18                307
#define TERR3D_KEY_F19                308
#define TERR3D_KEY_F20                309
#define TERR3D_KEY_F21                310
#define TERR3D_KEY_F22                311
#define TERR3D_KEY_F23                312
#define TERR3D_KEY_F24                313
#define TERR3D_KEY_F25                314
#define TERR3D_KEY_KP_0               320
#define TERR3D_KEY_KP_1               321
#define TERR3D_KEY_KP_2               322
#define TERR3D_KEY_KP_3               323
#define TERR3D_KEY_KP_4               324
#define TERR3D_KEY_KP_5               325
#define TERR3D_KEY_KP_6               326
#define TERR3D_KEY_KP_7               327
#define TERR3D_KEY_KP_8               328
#define TERR3D_KEY_KP_9               329
#define TERR3D_KEY_KP_DECIMAL         330
#define TERR3D_KEY_KP_DIVIDE          331
#define TERR3D_KEY_KP_MULTIPLY        332
#define TERR3D_KEY_KP_SUBTRACT        333
#define TERR3D_KEY_KP_ADD             334
#define TERR3D_KEY_KP_ENTER           335
#define TERR3D_KEY_KP_EQUAL           336
#define TERR3D_KEY_LEFT_SHIFT         340
#define TERR3D_KEY_LEFT_CONTROL       341
#define TERR3D_KEY_LEFT_ALT           342
#define TERR3D_KEY_LEFT_SUPER         343
#define TERR3D_KEY_RIGHT_SHIFT        344
#define TERR3D_KEY_RIGHT_CONTROL      345
#define TERR3D_KEY_RIGHT_ALT          346
#define TERR3D_KEY_RIGHT_SUPER        347
#define TERR3D_KEY_MENU               348

#define TERR3D_KEY_LAST               TERR3D_KEY_MENU