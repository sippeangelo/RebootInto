#include "Platform.h"
#include <sys/reboot.h>
extern "C" {
#include <efivar.h>
}

void Platform::Initialize() { }

bool Platform::UEFICheck()
{
	return efi_variables_supported();
}

std::pair<std::uint8_t*, std::size_t> Platform::UEFIReadVariable(const std::string& name)
{
    std::uint8_t* data = nullptr;
    std::size_t size;
    std::uint32_t attributes;
    if (efi_get_variable(EFI_GLOBAL_GUID, name.c_str(), &data, &size, &attributes) != 0) {
        std::stringstream message;
        message << "Failed to read UEFI variable \"" << name << "\"";
        throw std::runtime_error(message.str());
    }

    return std::make_pair(data, size);
}

void Platform::UEFIWriteVariable(const std::string& name, void* buffer, std::size_t size)
{
    if (efi_set_variable(EFI_GLOBAL_GUID, name.c_str(), static_cast<std::uint8_t*>(buffer), size, EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE, 0600) != 0) {
        throw std::runtime_error("Failed to write UEFI variable: BootNext");
    }
}

void Platform::Reboot() 
{
	sync();
	if (reboot(RB_AUTOBOOT) != -1) {
		throw std::runtime_error("Failed to initiate reboot.");
	}
}