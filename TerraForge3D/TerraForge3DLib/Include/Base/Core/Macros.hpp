
#define TF3D_GENERATION 3
#define TF3D_VERSION_MIN 0
#define TF3D_VERSION_MAX 0

#define TF3D_VERSION_STRING ( std::to_string(TF3D_GENERATION) + "." + std::to_string(TF3D_VERSION_MAX) + "." + std::to_string(TF3D_VERSION_MIN) )


#define TF3D_SAFE_DELETE(x) if(x) { delete x; x = nullptr; }

#define TF3D_HANDLE_EXCEPTION_MSG(x, message) { try {x;} catch(std::exception& e){TF3D_LOG_ERROR("{0} [ {1} ]", message, e.what());} }
#define TF3D_HANDLE_EXCEPTION(x) TF3D_HANDLE_EXCEPTION_MSG(x, "Exception:")

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

// Uncomment only one macro at a time here
#define TF3D_OPENGL_BACKEND
// #define TF3D_VULKAN_BACKEND

#ifdef TF3D_VULKAN_BACKEND
#define TF3D_BACKEND "Vulkan"
#elif defined(TF3D_OPENGL_BACKEND)
#define TF3D_BACKEND "OpenGL"
#else
#error "Unknown Backend"
#endif

#ifdef TF3D_WINDOWS

#define TF3D_PLATFORM "Windows"
#define PATH_SEPERATOR "\\"

#elif defined(TF3D_LINUX)

#define TF3D_PLATFORM "Linux"
#define PATH_SEPERATOR "/"

#elif defined(TF3D_MACOSX)

#define TF3D_PLATFORM "MacOS"
#define PATH_SEPERATOR "/"

#else

// #error "Unknown Platform"

#endif