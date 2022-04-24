
#define TF3D_GENERATION 3
#define TF3D_VERSION_MIN 0
#define TF3D_VERSION_MAX 0

#define TF3D_VERSION_STRING ( std::to_string(TF3D_GENERATION) + "." + std::to_string(TF3D_VERSION_MIN) + "." + std::to_string(TF3D_VERSION_MAX) )


#define TF3D_SAFE_DELETE(x) if(x) { delete x; x = nullptr; }

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

// Uncomment only one macro at a time here
#define TF3D_OPENGL_BACKEND
// #define TF3D_VULKAN_BACKEND