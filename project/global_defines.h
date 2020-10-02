// This is used for remote debugging, since the graphics can not be supported while remote debugging, you need to comment the define when you need remote debug

// #define DEBUG

#define REMOTE_DESKTOP

#define MAX_SIM_SIZE 40 // This is the max simulations count (AKA the max StarCraft II processes running symutaniously)

#ifdef REMOTE_DESKTOP
#define USE_SYSTEM_COMMAND
#else
#define REAL_TIME_UPDATE
#define USE_GRAPHICS
#define USE_SYSTEM_COMMAND
#endif

#define var2str(name) std::string((#name)) // convert a expr (variable, class...anything you write in these parentheses) to string
