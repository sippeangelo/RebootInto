#ifndef Platform_h__
#define Platform_h__

#include <stdexcept>
#include <sstream>
#include <boost/locale.hpp>

namespace Platform {

void Initialize();
bool UEFICheck();
std::pair<std::uint8_t*, std::size_t> UEFIReadVariable(const std::string& name);
void UEFIWriteVariable(const std::string& name, void* buffer, std::size_t size);
void Reboot();

}

#endif