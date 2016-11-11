#include "Platform.h"
#include <sstream>
#include <Windows.h>

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

void Platform::Reboot()
{
	if (InitiateSystemShutdown(nullptr, nullptr, 0, false, true) == 0) {
		std::stringstream message;
		message << "Failed to initiate reboot: " << GetLastError();
		throw std::runtime_error(message.str());
	}
}