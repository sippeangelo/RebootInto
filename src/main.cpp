#include <iostream>
#include <tclap/CmdLine.h>
#include <boost/optional.hpp>
#include "Platform.h"
#include "UEFI.h"

#define VERSION "2.0"
bool VERBOSE = false;

void printBootOrder(UEFI& uefi)
{
    auto order = uefi.ReadBootOrder();
    for (auto& option : order) {
        std::cout << option.ID << ": \"" << option.Description << "\"" << std::endl;
    }
}

int main(int argc, char* argv[])
{
	Platform::Initialize();
    UEFI uefi;

    // Arguments
    auto cmd = std::make_shared<TCLAP::CmdLine>("Tool to simplify dual-boot scenarios by changing the UEFI boot order to the desired entry and then reboot.", ' ', VERSION);
    TCLAP::SwitchArg arg_list("l", "list", "List current UEFI boot order and exit.");
    TCLAP::SwitchArg arg_current("c", "current", "Move the currently booted UEFI entry to the top of the boot order and exit. Useful if you want your manual UEFI choices to stick, by automatically running this argument at startup.");
    TCLAP::ValueArg<int> arg_offset("o", "offset", "Numerical offset from the top of the boot order that acts as a barrier to how high entries will be moved. Default is 0, which means boot entries will be moved to the very top. Useful if you want UEFI to try another media before proceeding with your chosen boot entry.", false, 0, "offset");
    TCLAP::SwitchArg arg_once("n", "once", "Change boot order for the next boot only");
    TCLAP::SwitchArg arg_noreboot("b", "noreboot", "Do not reboot.");
    TCLAP::SwitchArg arg_verbose("v", "verbose", "Verbose output.");
    TCLAP::UnlabeledValueArg<std::string> arg_entry("entry", "The UEFI entry to reboot into.", false, "", "UEFI entry");

    // Parse command line
    try {
        cmd->add(arg_list);
        cmd->add(arg_current);
        cmd->add(arg_offset);
        cmd->add(arg_once);
        cmd->add(arg_noreboot);
        cmd->add(arg_verbose);
        cmd->add(arg_entry);
        cmd->parse(argc, argv);
    } catch (TCLAP::ArgException& e) {
        std::cerr << "Error: " << e.error() << " for arg " << e.argId() << std::endl;
        return 1;
    }
    // Positional argument requirement
    if (arg_entry.getValue().empty() && !arg_list.getValue() && !arg_current.getValue()) {
        auto output = cmd->getOutput();
        output->usage(*cmd);
        return 1;
    }
    VERBOSE = arg_verbose.getValue();

    // Print boot order
    if (VERBOSE) { // --verbose
        std::cout << "Current boot order:" << std::endl;
    }
    if (VERBOSE || arg_list.getValue()) { // --verbose --list
        printBootOrder(uefi);
    }
    if (arg_list.getValue()) { // --list
        return 0;
    }

    // Fetch boot order
    auto order = uefi.ReadBootOrder();

    // Get option ID
    std::vector<UEFI::BootOption>::iterator option = order.end();
    if (arg_current.getValue()) { // --current
        UEFI::BootOption current = uefi.ReadBootCurrent();
        option = std::find_if(order.begin(), order.end(), [&current](auto& o) { return o.ID == current.ID; });
        if (option == order.end()) {
            std::cerr << "Error: Currently booted UEFI entry has been deleted since last boot!" << std::endl;
            return 1;
        }
        if (VERBOSE) { // --verbose
            std::cout << "Currently booted UEFI entry is \"" << (*option).ID << ": " << (*option).Description << "\"" << std::endl;
        }
    } else { // <UEFI entry>
        // Find by ID
        try {
            std::size_t numNums;
            std::uint16_t id = std::stoi(arg_entry.getValue(), &numNums);
            if (numNums == arg_entry.getValue().size()) {
                option = std::find_if(order.begin(), order.end(), [&id](auto& o) { return o.ID == id; });
                if (VERBOSE && option != order.end()) { // --verbose
                    std::cout << "UEFI entry matched by ID." << std::endl;
                }
            }
        } catch (std::invalid_argument& e) {
        } catch (std::out_of_range& e) { }

        // Find by description if ID didn't match already
        if (option == order.end()) {
            auto& identifier = arg_entry.getValue();
            option = std::find_if(order.begin(), order.end(), [&identifier](auto& o) { return o.Description == identifier; });
            if (VERBOSE && option != order.end()) { // --verbose
                std::cout << "UEFI entry matched by description." << std::endl;
            }
        }

        if (option == order.end()) {
            std::cerr << "Error: No UEFI entry found with ID or description \"" << arg_entry.getValue() << "\"." << std::endl;
            std::cerr << "Use --list to list all available entries." << std::endl;
            return 1;
        }
    }

    // Write boot commands to UEFI
    if (arg_once.getValue()) { // --once
        // Set BootNext
        uefi.WriteBootNext(*option);
        std::cout << "BootNext changed successfully." << std::endl;
    } else {
        // Move option to top of boot order
        auto currentDefault = order.begin();
        std::advance(currentDefault, arg_offset.getValue()); // --offset=0
        auto newDefault = option;
        if (newDefault > currentDefault) {
            // Shift entry to top
            UEFI::BootOption tmp = *newDefault;
            std::copy_backward(currentDefault, newDefault, std::next(newDefault));
            *currentDefault = tmp;

            // Apply boot order
            uefi.WriteBootOrder(order);

            std::cout << "Boot order changed successfully." << std::endl;
        } else if (newDefault < currentDefault) {
            std::cout << "Boot entry is already above provided offset! Nothing has been changed." << std::endl;
        } else {
            std::cout << "Boot entry is already the default! Nothing has been changed." << std::endl;
        }
    }

    if (VERBOSE) { // --verbose
        std::cout << "New boot order:" << std::endl;
        printBootOrder(uefi);
    }

    // Reboot
    if (!arg_noreboot.getValue() && !arg_current.getValue()) { // --noreboot --current
        std::cout << "Rebooting..." << std::endl;
		Platform::Reboot();
    }

    return 0;
}