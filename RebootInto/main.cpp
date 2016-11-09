#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <map>
#include <array>
#include <vector>
#include <memory>
#define NOMINMAX
#include <Windows.h>
#include <nowide/convert.hpp>
//#define TCLAP_NAMESTARTSTRING "/"
//#define TCLAP_FLAGSTARTSTRING "/"
#include <tclap/CmdLine.h>

using namespace nowide;

#define VERSION "1.0.1"
#define EFI_GLOBAL_VARIABLE L"{8BE4DF61-93CA-11D2-AA0D-00E098032B8C}"
const std::size_t EFI_LOAD_OPTION_DESCRIPTION_OFFSET = sizeof(std::uint32_t) + sizeof(std::uint16_t);
const std::uint16_t INVALID_OPTION_ID = std::numeric_limits<std::uint16_t>::max();

bool Verbose = false;
std::vector<std::uint16_t> BootOrder;
std::map<std::string, std::uint16_t> DescriptionToID;
std::map<std::uint16_t, std::string> IDToDescription;

DWORD SetPrivilege(HANDLE hToken, LPCWSTR lpszPrivilege, bool bEnablePrivilege)
{
	LUID luid;
	if (!LookupPrivilegeValueW(NULL, lpszPrivilege, &luid)) {
		std::cerr << "LookupPrivilegeValue failed: " << GetLastError() << std::endl;
		return GetLastError();
	}

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege) {
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	} else {
		tp.Privileges[0].Attributes = 0;
	}

	if (!AdjustTokenPrivileges(
		hToken,
		false,
		&tp,
		sizeof(TOKEN_PRIVILEGES),
		nullptr,
		nullptr
	)) {
		std::cerr << "AdjustTokenPrivileges failed: " << GetLastError() << std::endl;
		return GetLastError();
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		std::cerr << "AdjustTokenPrivileges failed: The token does not have the specified privilege" << std::endl;
		return GetLastError();
	}

	return ERROR_SUCCESS;
}

DWORD SetPrivileges()
{
	// Get access token
	HANDLE hInstance = GetCurrentProcess();
	HANDLE hToken;
	if (OpenProcessToken(hInstance, TOKEN_ADJUST_PRIVILEGES, &hToken) == 0) {
		std::cerr << "OpenProcessToken failed: " << GetLastError() << std::endl;
		return GetLastError();
	}

	DWORD result;
	// Enable environment edit privileges
	result = SetPrivilege(hToken, SE_SYSTEM_ENVIRONMENT_NAME, true);
	if (result != ERROR_SUCCESS) {
		return result;
	}
	// Enable shutdown privileges
	result = SetPrivilege(hToken, SE_SHUTDOWN_NAME, true);
	if (result != ERROR_SUCCESS) {
		return result;
	}

	return ERROR_SUCCESS;
}

DWORD LoadBootOrder()
{
	std::array<std::uint16_t, 32> buffer;
	std::size_t size = GetFirmwareEnvironmentVariableW(L"BootOrder", EFI_GLOBAL_VARIABLE, buffer.data(), sizeof(std::uint16_t) * buffer.size());
	if (size == 0) {
		std::cout << "LoadBootOrder failed: " << GetLastError() << std::endl;
		return GetLastError();
	}

	int numEntries = size / sizeof(std::uint16_t);
	if (Verbose) {
		std::cout << "Received " << numEntries << " UEFI entries." << std::endl;
	}

	BootOrder.resize(numEntries);
	std::copy_n(buffer.begin(), numEntries, BootOrder.begin());

	return ERROR_SUCCESS;
}

DWORD LoadBootDescriptions()
{
	char* buffer = new char[1024];
	for (int i = 0; i < BootOrder.size(); i++) {
		std::uint16_t id = BootOrder.at(i);
		char entry[9] = "Boot####";
		std::snprintf(entry, 9, "Boot%04X", id);

		std::size_t size = GetFirmwareEnvironmentVariableW(widen(entry).c_str(), EFI_GLOBAL_VARIABLE, buffer, sizeof(char) * 1024);
		if (size == 0) {
			std::cerr << "LoadBootDescriptions failed on " << entry << ": " << GetLastError() << std::endl;
			return GetLastError();
		}

		std::string description(narrow(reinterpret_cast<wchar_t*>(buffer + EFI_LOAD_OPTION_DESCRIPTION_OFFSET)));
		DescriptionToID[description] = id;
		IDToDescription[id] = description;
	}
	delete[] buffer;

	return ERROR_SUCCESS;
}

void PrintBootOrder() 
{
	for (auto& id : BootOrder) {
		std::cout << id << ": \"" << IDToDescription.at(id) << "\"" << std::endl;
	}
}

std::uint16_t GetCurrentBootOption()
{
	std::uint16_t current;
	std::size_t size = GetFirmwareEnvironmentVariableW(L"BootCurrent", EFI_GLOBAL_VARIABLE, &current, sizeof(std::uint16_t));
	if (size == 0) {
		std::cout << "GetCurrentBootOption failed: " << GetLastError() << std::endl;
		return INVALID_OPTION_ID;
	}
	return current;
}

DWORD SetBootNext(std::uint16_t entry)
{
	bool result = SetFirmwareEnvironmentVariableW(L"BootNext", EFI_GLOBAL_VARIABLE, &entry, sizeof(std::uint16_t));
	if (!result) {
		std::cerr << "SetBootNext failed: " << GetLastError() << std::endl;
		return GetLastError();
	}

	return ERROR_SUCCESS;
}

DWORD SetBootDefault(std::uint16_t entry, std::size_t offset = 0)
{
	auto currentDefault = BootOrder.begin();
	std::advance(currentDefault, offset);
	auto newDefault = std::find(BootOrder.begin(), BootOrder.end(), entry);
	if (newDefault > currentDefault) {
		// Shift entry to top
		std::uint16_t tmp = *newDefault;
		std::copy_backward(currentDefault, newDefault, std::next(newDefault));
		*currentDefault = tmp;

		// Apply boot order
		bool result = SetFirmwareEnvironmentVariableW(L"BootOrder", EFI_GLOBAL_VARIABLE, BootOrder.data(), sizeof(std::uint16_t) * BootOrder.size());
		if (!result) {
			std::cerr << "Failed to apply boot order: " << GetLastError() << std::endl;
			return GetLastError();
		}

		std::cout << "Boot order changed successfully." << std::endl;
	} else if (newDefault < currentDefault) {
		std::cout << "Boot entry is already above provided offset! Nothing has been changed." << std::endl;
	} else {
		std::cout << "Boot entry is already the default! Nothing has been changed." << std::endl;
	}

	return ERROR_SUCCESS;
}

int wmain(int argc, wchar_t* argv[])
{
	// Check for UEFI
	FIRMWARE_TYPE firmwareType;
	if (GetFirmwareType(&firmwareType) == 0) {
		std::cerr << "GetFirmwareType failed: " << GetLastError() << std::endl;
		return 1;
	}
	if (firmwareType != FirmwareTypeUefi) {
		std::cerr << "Error: System is not UEFI!" << std::endl;
		return 1;
	}

	// Create a UTF-8 argument vector for TCLAP
	std::vector<std::string> args(argc);
	for (int i = 0; i < argc; i++) {
		args[i] = narrow(argv[i]);
	}

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
		cmd->parse(args);
	} catch (TCLAP::ArgException& e) {
		std::cerr << "Error: " << e.error() << " for arg " << e.argId() << std::endl;
		return 1;
	}
	Verbose = arg_verbose.getValue();
	// Positional argument requirement
	if (arg_entry.getValue().empty() && !arg_list.getValue() && !arg_current.getValue()) {
		auto output = cmd->getOutput();
		output->usage(*cmd);
		return 1;
	}

	// Initialize
	if (SetPrivileges() != ERROR_SUCCESS) { return 1; }
	if (LoadBootOrder() != ERROR_SUCCESS) { return 1; }
	if (LoadBootDescriptions() != ERROR_SUCCESS) { return 1; }

	// Print boot order
	if (arg_list.getValue() || Verbose) { // --list --verbose
		PrintBootOrder();
		if (!Verbose) {
			return 0;
		}
	}

	// Get option ID
	std::uint16_t entry = INVALID_OPTION_ID;
	if (arg_current.getValue()) { // --current
		entry = GetCurrentBootOption();
		if (entry == INVALID_OPTION_ID) {
			return 1;
		}
		if (std::find(BootOrder.begin(), BootOrder.end(), entry) == BootOrder.end()) {
			std::cerr << "Error: Currently booted UEFI entry has been deleted since last boot!" << std::endl;
			return 1;
		}
		if (Verbose) {
			std::cout << "Currently booted UEFI entry is \"" << entry << ": " << IDToDescription.at(entry) << "\"" << std::endl;
		}
	} else { // <UEFI entry>
		// Find by ID
		try {
			std::size_t numNums;
			std::uint16_t id = std::stoi(arg_entry.getValue(), &numNums);
			if (numNums == arg_entry.getValue().size()) {
				auto it = std::find(BootOrder.begin(), BootOrder.end(), id);
				if (it != BootOrder.end()) {
					entry = id;
					if (Verbose) {
						std::cout << "UEFI entry matched by ID." << std::endl;
					}
				}
			}
		} catch (std::invalid_argument& e) { 
		} catch (std::out_of_range& e) { }
		
		// Find by description
		if (entry == INVALID_OPTION_ID) {
			auto it = DescriptionToID.find(arg_entry.getValue());
			if (it == DescriptionToID.end()) {
				std::cerr << "Error: No UEFI entry found with ID or description \"" << arg_entry.getValue() << "\"." << std::endl;
				std::cerr << "Use --list to list all available entries." << std::endl;
				return 1;
			}

			if (Verbose) {
				std::cout << "UEFI entry matched by description." << std::endl;
			}
			entry = it->second;
		}
	}

	// Write boot commands to UEFI
	if (arg_once.getValue()) { // --once
		// Set BootNext
		if (SetBootNext(entry) != ERROR_SUCCESS) { return 1; }
	} else {
		// Move option to top of boot order
		if (SetBootDefault(entry, arg_offset.getValue()) != ERROR_SUCCESS) { return 1; }
	}

	// Reboot
	if (!arg_noreboot.getValue() && !arg_current.getValue()) { // --noreboot --current
		if (InitiateSystemShutdown(nullptr, nullptr, 0, false, true) == 0) {
			std::cerr << "Failed to initiate reboot: " << GetLastError() << std::endl;
			return 1;
		} else {
			std::cout << "Rebooting..." << std::endl;
		}
	}

	return 0;
}