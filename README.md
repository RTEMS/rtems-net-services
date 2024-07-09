RTEMS Network Services
======================

The rtems-net-services repository serves as a central location to manage
libraries and tests that require network support, but can otherwise be shared
across multiple network stacks.


File Origins
------------
The files under the bsd/ directory are sourced from:

  https://github.com/sebhub/rtems-libbsd.git

Commits adding files should include the hash of the target repository if 
applicable.



Installation Instructions
-------------------------
  1. Populate the git submodules:

     ```shell
     git submodule init
     git submodule update
     ```
  2. Configure and build

     ```shell
     ./waf configure --prefix=INSTALL_PREFIX
     ./waf
     ./waf install
     ```

     More `waf` arguments can be found by using:

     ```shell
     `./waf --help`
     ```

Further Build Information
-------------------------

The BSPs configured to build may be specified on the waf configure command line
with --rtems-bsps or they may be configured in config.ini as in RTEMS. The
command line option will override the BSPs configured in config.ini.
