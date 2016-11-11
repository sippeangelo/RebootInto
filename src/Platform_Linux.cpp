#include "Platform.h"
#include <unistd.h>
#include <sys/reboot.h>

void Platform::Initialize() 
{
	// TODO: root permission check here
}

void Platform::Reboot() 
{
	sync();
	if (reboot(RB_AUTOBOOT) != -1) {
		throw std::runtime_error("Failed to initiate reboot.");
	}
}