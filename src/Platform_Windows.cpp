#include "Platform.h"
#include <Windows.h>

const LPCWSTR EFI_GLOBAL_GUID = L"{8BE4DF61-93CA-11D2-AA0D-00E098032B8C}";

void SetPrivilege(HANDLE hToken, LPCWSTR lpszPrivilege, bool bEnablePrivilege)
{
	LUID luid;
	if (!LookupPrivilegeValueW(NULL, lpszPrivilege, &luid)) {
		std::stringstream message;
		message << "LookupPrivilegeValue failed: " << GetLastError();
		throw std::runtime_error(message.str());
	}

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege) {
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	} else {
		tp.Privileges[0].Attributes = 0;
	}

	if (!AdjustTokenPrivileges(hToken, false, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr)) {
		std::stringstream message;
		message << "AdjustTockenPrivileges failed: " << GetLastError();
		throw std::runtime_error(message.str());
	}
}

void Platform::Initialize()
{
	// Get access token
	HANDLE hInstance = GetCurrentProcess();
	HANDLE hToken;
	if (OpenProcessToken(hInstance, TOKEN_ADJUST_PRIVILEGES, &hToken) == 0) {
		std::stringstream message;
		message << "OpenProcessToken failed: " << GetLastError();
		throw std::runtime_error(message.str());
	}

	// Enable environment edit privileges
	SetPrivilege(hToken, SE_SYSTEM_ENVIRONMENT_NAME, true);
	// Enable shutdown privileges
	SetPrivilege(hToken, SE_SHUTDOWN_NAME, true);
}

bool Platform::UEFICheck()
{
	FIRMWARE_TYPE firmwareType;
	if (GetFirmwareType(&firmwareType) == 0) {
		std::stringstream message;
		message << "GetFirmwareType failed: " << GetLastError();
		throw std::runtime_error(message.str());
	}

	return firmwareType == FirmwareTypeUefi;
}

std::pair<std::uint8_t*, std::size_t> Platform::UEFIReadVariable(const std::string& name)
{
	auto nameUTF16 = boost::locale::conv::utf_to_utf<char16_t, char>(name);

	const std::size_t bufferSize = 2048;
    std::uint8_t* data = std::malloc(bufferSize);
	std::size_t size = GetFirmwareEnvironmentVariableW(reinterpret_cast<LPCWSTR>(nameUTF16.c_str()), EFI_GLOBAL_GUID, data, sizeof(std::uint8_t) * bufferSize);
	if (size == 0) {
		std::stringstream message;
		message << "Failed to read UEFI variable \"" << name << "\": " << GetLastError();
		throw std::runtime_error(message.str());
	}
    return std::make_pair(data, size);
}

void Platform::UEFIWriteVariable(const std::string& name, void* buffer, std::size_t size)
{
	auto nameUTF16 = boost::locale::conv::utf_to_utf<char16_t, char>(name);

	bool result = SetFirmwareEnvironmentVariableW(reinterpret_cast<LPCWSTR>(nameUTF16.c_str()), EFI_GLOBAL_GUID, buffer, size);
	if (!result) {
		std::stringstream message;
		message << "Failed to write UEFI variable \"" << name << "\": " << GetLastError();
		throw std::runtime_error(message.str());
	}
}

void Platform::Reboot()
{
	if (InitiateSystemShutdown(nullptr, nullptr, 0, false, true) == 0) {
		std::stringstream message;
		message << "Failed to initiate reboot: " << GetLastError();
		throw std::runtime_error(message.str());
	}
}