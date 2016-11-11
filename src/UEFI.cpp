#include "UEFI.h"

UEFI::UEFI()
{
	if (!Platform::UEFICheck()) {
        throw std::runtime_error("System is not UEFI!");
	}
}

UEFI::BootOption UEFI::ReadBootCurrent()
{
    auto data = Platform::UEFIReadVariable("BootCurrent");

    UEFI::BootOption option;
	option.ID = *reinterpret_cast<std::uint16_t*>(data.first);
    option.Description = readDescription(option.ID);

    std::free(data.first);

    return option;
}

UEFI::BootOption UEFI::ReadBootNext()
{
    auto data = Platform::UEFIReadVariable("BootNext");

    UEFI::BootOption option;
    option.ID = *reinterpret_cast<std::uint16_t*>(data.first);
    option.Description = readDescription(option.ID);

    std::free(data.first);

    return option;
}

void UEFI::WriteBootNext(const UEFI::BootOption& option)
{
	auto data = const_cast<std::uint16_t*>(&option.ID);
	Platform::UEFIWriteVariable("BootNext", data, sizeof(std::uint16_t));
}

std::vector<UEFI::BootOption> UEFI::ReadBootOrder()
{
    auto data = Platform::UEFIReadVariable("BootOrder");

    int numOptions = data.second / sizeof(std::uint16_t);
    std::vector<UEFI::BootOption> bootOrder(numOptions);
    for (int i = 0; i < numOptions; ++i) {
        auto& option = bootOrder[i];
        option.ID = *(reinterpret_cast<std::uint16_t*>(data.first) + i);
        option.Description = readDescription(option.ID);
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

	Platform::UEFIWriteVariable("BootOrder", data.data(), sizeof(std::uint16_t) * data.size());
}

std::string UEFI::readDescription(std::uint16_t id)
{
    auto data = Platform::UEFIReadVariable(optionVarFromID(id));

    char16_t* descUTF16 = reinterpret_cast<char16_t*>(data.first + EFI_LOAD_OPTION_DESCRIPTION_OFFSET);
    std::string descUTF8 = boost::locale::conv::utf_to_utf<char, char16_t>(descUTF16);

    std::free(data.first);

    return descUTF8;
}

std::string UEFI::optionVarFromID(std::uint16_t id)
{
    char entry[9] = "Boot####";
    std::snprintf(entry, 9, "Boot%04X", id);
    return std::string(entry);
}
