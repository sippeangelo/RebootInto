#include <cstdint>
#include <limits>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>

const std::size_t EFI_LOAD_OPTION_DESCRIPTION_OFFSET = sizeof(std::uint32_t) + sizeof(std::uint16_t);

class UEFI
{
public:
    struct BootOption
    {
        std::uint16_t ID;
        std::string Description;
    };

    UEFI();

    BootOption ReadBootCurrent();
    BootOption ReadBootNext();
    void WriteBootNext(const BootOption& option);
    std::vector<BootOption> ReadBootOrder();
    void WriteBootOrder(const std::vector<BootOption>& bootOrder);

private:
    std::string ReadDescription(std::uint16_t id);
    std::string VarNameFromID(std::uint16_t id);
};

