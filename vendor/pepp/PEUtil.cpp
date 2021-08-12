#include "PELibrary.hpp"

#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")

using namespace pepp;

std::string pepp::DemangleName(std::string_view mangled_name)
{
    //
    // TODO: Don't rely on DbgHelp??
    char undecorated_name[1024];
    UnDecorateSymbolName(
        mangled_name.data(),
        undecorated_name,
        sizeof undecorated_name,
        UNDNAME_32_BIT_DECODE | UNDNAME_NAME_ONLY);
    
    return undecorated_name;
}

