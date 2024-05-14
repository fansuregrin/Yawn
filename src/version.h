#ifndef VERSION_H
#define VERSION_H

#include <string>

#define _VENDOR_NAME "yawn"
#define _VER_MAJOR 0
#define _VER_MINOR 0
#define _VER_PATCH 0

#define _VERSION_NUMBER ((_VER_MAJOR) * 10000 + (_VER_MINOR) * 100 + (_VER_PATCH))
#define _VERSION_STRING (\
    std::to_string(_VER_MAJOR) + '.' +\
    std::to_string(_VER_MINOR) + '.' +\
    std::to_string(_VER_PATCH))

#endif // VERSION_H