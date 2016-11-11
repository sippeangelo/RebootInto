#include "UEFI.h"
#include <cstdlib>
#include <sstream>
#include <boost/locale.hpp>
extern "C" {
    #include <efivar.h>
}

template <typename T>
std::pair<T*, std::size_t> ReadVariable(const std::string& name)
{
    std::uint8_t* data = nullptr;
    std::size_t size;
    std::uint32_t attributes;
    if (efi_get_variable(EFI_GLOBAL_GUID, name.c_str(), &data, &size, &attributes) != 0) {
        std::stringstream message;
        message << "Failed to read EFI variable: " << name;
        throw std::runtime_error(message.str());
    }

    return std::make_pair(reinterpret_cast<T*>(data), size);
}

UEFI::UEFI()
{
    efi_error_clear();
    if (!efi_variables_supported()) {
        throw std::runtime_error("System is not UEFI!");
    }
}

UEFI::BootOption UEFI::ReadBootCurrent()
{
    auto data = ReadVariable<std::uint16_t>("BootCurrent");

    UEFI::BootOption option;
    option.ID = *data.first;
    option.Description = ReadDescription(option.ID);

    std::free(data.first);

    return option;
}

UEFI::BootOption UEFI::ReadBootNext()
{
    auto data = ReadVariable<std::uint16_t>("BootNext");

    UEFI::BootOption option;
    option.ID = *data.first;
    option.Description = ReadDescription(option.ID);

    std::free(data.first);

    return option;
}

void UEFI::WriteBootNext(const UEFI::BootOption& option)
{
    if (efi_set_variable(EFI_GLOBAL_GUID, "BootNext", reinterpret_cast<std::uint8_t*>(const_cast<std::uint16_t*>(&option.ID)), sizeof(std::uint16_t), EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE, 0600) != 0) {
        throw std::runtime_error("Failed to set EFI variable: BootNext");
    }
}

std::vector<UEFI::BootOption> UEFI::ReadBootOrder()
{
    auto data = ReadVariable<std::uint16_t>("BootOrder");

    int numOptions = data.second / sizeof(std::uint16_t);
    std::vector<UEFI::BootOption> bootOrder(numOptions);
    for (int i = 0; i < numOptions; ++i) {
        auto& option = bootOrder[i];
        option.ID = *(data.first + i);
        option.Description = ReadDescription(option.ID);
    }

    std::free(data.first);

    return bootOrder;
}

void UEFI::WriteBootOrder(const std::vector<UEFI::BootOption>& order)
{
    std::vector<std::uint16_t> data(order.size());
    for (int i = 0; i < order.size(); ++i) {
        data[i] = order[i].ID;
    }

    if (efi_set_variable(EFI_GLOBAL_GUID, "BootOrder", reinterpret_cast<std::uint8_t*>(data.data()), sizeof(std::uint16_t) * data.size(), EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE, 0600) != 0) {
        throw std::runtime_error("Failed to set EFI variable: BootOrder");
    }
}

std::string UEFI::ReadDescription(std::uint16_t id)
{
    auto data = ReadVariable<char>(VarNameFromID(id));

    char16_t* descUTF16 = reinterpret_cast<char16_t*>(data.first + EFI_LOAD_OPTION_DESCRIPTION_OFFSET);
    std::string descUTF8 = boost::locale::conv::utf_to_utf<char, char16_t>(descUTF16);

    std::free(data.first);

    return descUTF8;
}

std::string UEFI::VarNameFromID(std::uint16_t id)
{
    char entry[9] = "Boot####";
    std::snprintf(entry, 9, "Boot%04X", id);
    return std::string(entry);
}
