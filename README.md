# RebootInto
**RebootInto is a Windows and Linux compatible utility to modify the UEFI boot order before a reboot.** 
Its purpose is to simplify a dual-boot scenario by modifying the UEFI boot order
before rebooting, letting you boot into a different operating system without 
touching your keyboard.


```
USAGE:

   rebootinto    [-v] [-b] [-n] [-o <offset>] [-c] [-l] [--] [--version]
                 [-h] <UEFI entry>


Where:

   -v,  --verbose
     Verbose output.

   -b,  --noreboot
     Do not reboot.

   -n,  --once
     Change boot order for the next boot only

   -o <offset>,  --offset <offset>
     Numerical offset from the top of the boot order that acts as a barrier
     to how high entries will be moved. Default is 0, which means boot
     entries will be moved to the very top. Useful if you want UEFI to try
     another media before proceeding with your chosen boot entry.

   -c,  --current
     Move the currently booted UEFI entry to the top of the boot order and
     exit. Useful if you want your manual UEFI choices to stick, by
     automatically running this argument at startup.

   -l,  --list
     List current UEFI boot order and exit.

   --,  --ignore_rest
     Ignores the rest of the labeled arguments following this flag.

   --version
     Displays version information and exits.

   -h,  --help
     Displays usage information and exits.

   <UEFI entry>
     The UEFI entry to reboot into.
```
