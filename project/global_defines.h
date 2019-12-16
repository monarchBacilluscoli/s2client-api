// This is used for remote debugging, since the graphics can not be supported while remote debugging, you need to comment the define when you need remote debug

// #define REMOTE_DESKTOP

#ifdef REMOTE_DESKTOP
#else
#define USE_GRAPHICS
#define USE_SYSTEM_COMMAND
#endif

#define var2str(name) std::string((#name)) // convert a variable's name to string
