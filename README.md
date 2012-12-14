NOVA runtime environment (NRE)
==============================

NRE is supposed to be the successor of NUL (NOVA UserLand). It has the goal
to be much simpler and cleaner than NUL, because NUL is quite complicated and
difficult to maintain. This is achieved by providing easy-to-use abstractions,
using C++ exception handling and the well known pattern RAII.
Another important difference to NUL is that each service (typically a
component that would be a single or a group of host-drivers in NUL) runs in
its own protection domain to make it more reliable and reduce the dependencies
between them. Additionally, the architecture x86_64 (besides x86_32) is
supported from the beginning.


Getting started:
----------------

Please take the following steps:

1.  Go into the directory cross/.
2.  a) If you want to download a precompiled cross-compiler, execute
    `download.sh <arch>`, where `<arch>` is either `x86_32` or `x86_64`.
    It will download the compiler into `/opt/nre-cross-<arch>`. There are only
    precompiled versions for certain host platforms, of course.  
    b) Otherwise, execute `build.sh <arch>`, where `<arch>` is either `x86_32`
    or `x86_64`. This builds the cross compiler. Thus, it will take a few
    minutes :)
3.  To checkout NOVA, please do a `git submodule init && git submodule update`
    in the root directory of the repository.
4.  Before you continue, choose the target and build-type that you want to use:  
    `export NRE_TARGET=(x86_32|x86_64) NRE_BUILD=(debug|release)`
5.  Go into the directory nre. You'll find the convenience-script b, which is
    responsible for building everything that needs to be build and executing
    the specified command. So, simply do a `./b` to build everything. It will
    build NOVA and, of course, build NRE as well. Please run `./b -h` to see
    the help of the script. For a quick start use `./b qemu boot/test` to run
    the test application in qemu.
6.  To use `./b srv <script>`, you have to generate the novaboot-config by:  
    `./tools/novaboot --dump-config > ~/.novaboot`  
    You might want to adjust it afterwards

Note that there are few boot-scripts that require additional binaries and
images. You can download them via `./dist/download.sh` in the nre directory. They
will be put into dist/imgs/. You have to download quite a few MB and it will
probably get more over time, which is the reason it is not included in the
repo. Similarly, you have to invoke `./dist/build.sh` to build a few test
harddisks and iso images first to be able to use a few boot scripts that
reference them.


Keybindings:
------------

NRE provides a console service that allows multiple applications to write to
the screen (not at the same time, of course). To do this, they request a sub-
console in a specified console. A console groups multiple subconsoles together,
so that e.g. all management stuff (Bootloader, Hypervisor, sysinfo, ...) can
be put in one console and each VM can be put in a different one. The console
service uses the following keybindings, where the modifier (ctrl by default)
can also be changed via modifier=x in the cmdline of console:

* ctrl + left/right: switch between consoles
* ctrl + up/down: switch between subconsoles
* ctrl + 0..9: switch to console 0..9. You can also use esc to get to console 0
* ctrl + end: reboot the host-machine


Debugging:
----------

In case you experience a problem and would like to debug it, here are a few
general suggestions on how to hunt it down:

* Do always use the debug build of NRE to find errors, as long as they occur in
  debug mode.
* You can enable/disable logging output in nre/include/Logging.h. Especially,
  it might be helpful to turn on EXCEPTIONS in order to see every thrown
  exception.
* It might also help to enable logging in NOVA by enabling some log-types in
  kernel/nova/include/stdio.h.
* To remote debug the system running in qemu via gdb, you can use
  `./b dbg=<app> <bootscript>`. This will start `<bootscript>` and use the
  debugging symbols of `<app>`. Since every NRE application is linked to a
  different address you shouldn't interfere with others.


Reporting errors:
-----------------

If you encounter a bug, you'll often see a backtrace that looks like this:

> [4] Backtrace:
> [4] 0000:0003:0030:a4d2
> [4] 0000:0003:0030:2210
> [4] 0000:0003:0030:fd5c

You can start `./b trace=<app>`, where `<app>` is e.g. `root`, and simply paste
the backtrace in there to decode it. Please always post the decoded backtrace,
because the addresses might map to different code for different people.
Errors should be reported via the [issue tracker on github](https://github.com/TUD-OS/NRE/issues).


Limitations:
------------

Please note in general, that NRE is still a young project and a work in
progress. So, there will probably be bugs and a lot of missing features.
Additionally, don't expect that the API stays stable.

The following lists a few known issues and limitations that you might run into:

* The network models of vancouver have not been ported yet and a network service
  in NRE is missing as well.
* It is not possible yet to get direct access to a PCI device.
* The storage service of NRE supports ATAPI drives, but vancouver does not.
* When using the storage service on real hardware, DMA with ATA/ATAPI drives
  might be an issue. You can turn it off by adding the parameter "noidedma" to
  storage

