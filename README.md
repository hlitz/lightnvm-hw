# lightnvm-hw
A Hello-World for LightNVM, useful tools for both QEMU and real
hardware implementations of the LightNVM (OpenChannel SSD) project.

## config

This folder contains config files for various incarnations of the
lightnvm linux kernel. The naming convnention is config.<sha> where
<sha> is the start of the sha key for the linux-lightnnvm fork of the
main link kernel (https://github.com/OpenChannelSSD/linux). 

There is also a simple build shell script with rules on how to build
the kernel on debian/ubuntu based systems.

## qemu

This folder contains some useful scripts for managing the lighnvm fork
of QEMU to enable testing without physical lightnvm compliant block
devices. The QEMU fork is at
https://github.com/OpenChannelSSD/qemu-nvme.

## nvme-cli

This is a submodule pointing to the lightnvm branch of the nvme-cli
project. Refer to the README in the submodule for more information. 

## sanity

This is a sanity test ;-). Run the python script called lnvm_test.py
with appropriate arguments to create and test lightnvm block devices that
use kernel plug-ins as the FTL. 'python lnvm_test.py -h' gives an
overview of the available commands.Sanity uses the most excellent fio tool
in order to test the created block device.

Useful arguments are: <br />
-g: Execute a set of auto-generated fio tests <br />
-s: Execute any fio tests (*.fio) in fio_tests/

A bash script called sanity.sh is also provided as an alternative for
single fio tests.

Note that both scripts must be launched with root privileges due to fio

Finally, the media manager sanity tests depend on liblightnvm [1]. Until
liblightnvm is provided as a packet, we have a git submodule for it, which we
update and install before the tests are executed.

[1] https://github.com/OpenChannelSSD/liblightnvm

# Contributions

Pull requests and patches gratefully received ;-).
