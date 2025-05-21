/**
 * @file
 *
 * @brief Code used to test the NTP deamon.
 */

/*
 * Copyright (C) 2022 embedded brains GmbH (http://www.embedded-brains.de)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/stat.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>

#include <rtems/console.h>
#include <rtems/imfs.h>
#include <rtems/ntpd.h>
#include <rtems/shell.h>
#include <rtems/test-printer.h>

#include <rtems/shellconfig-net-services.h>

#include <net_adapter.h>
#include <net_adapter_extra.h>
#include <network-config.h>

#include <rtems/ntpd.h>
#include <rtems/telnetd.h>

#include <tmacros.h>

#define DEBUGGER 0
#if DEBUGGER
#include <rtems/rtems-debugger.h>
#include <rtems/rtems-debugger-remote-tcp.h>
#endif /* DEBUGGER */

const char rtems_test_name[] = "NTP 1";

rtems_shell_env_t env;

static void telnet_shell( char *name, void *arg )
{
  rtems_shell_dup_current_env( &env );

  env.devname = name;
  env.taskname = "NTPD";

  rtems_shell_main_loop( &env );
}

rtems_telnetd_config_table rtems_telnetd_config = {
  .command = telnet_shell,
  .stack_size = 8 * RTEMS_MINIMUM_STACK_SIZE,
};

#define NTP_DEBUG 0
#define ntp_xstr(s) ntp_str(s)
#define ntp_str(s) #s
#define NTP_DEBUG_STR ntp_xstr(NTP_DEBUG)

static const char etc_resolv_conf[] =
    "nameserver " NET_CFG_DNS_IP "\n";

static atomic_bool ntp_start;
static int ntp_run_count;
static rtems_id ntpd_id;

static void debugger_start(void) {
#if DEBUGGER
  rtems_printer printer;
  int r;
  rtems_print_printer_fprintf(&printer, stdout);
  r = rtems_debugger_register_tcp_remote();
  if (r < 0) {
    printf("error: dserver: tcp remote register: %s", strerror(errno));
    return;
  }
  r = rtems_debugger_start(
    "tcp", "1122", RTEMS_DEBUGGER_TIMEOUT, 1, &printer);
  if (r < 0) {
    printf("error: dserver: failed to start\n");
    return;
  }
#endif /* DEBUGGER */
}

static void debugger_break(void) {
#if DEBUGGER
  printf("debugger: waiting ...   ");
  fflush(stdout);
  rtems_debugger_break(true);
  printf("\n");
#endif /* DEBUGGER */
}

static void setup_etc(void)
{
  int rv;

  /* FIXME: no direct IMFS */

  rv = IMFS_make_linearfile("/etc/resolv.conf", S_IWUSR | S_IRUSR |
      S_IRGRP | S_IROTH, etc_resolv_conf, sizeof(etc_resolv_conf));
  assert(rv == 0);
  rv = rtems_ntpd_client_pool_config(NET_CFG_NTP_IP);
  assert(rv == 0);
  rv = rtems_ntpd_add_etc_services();
  assert(rv == 0);
}

static void ntp_wait_until_running(void) {
  while (!rtems_ntpd_running()) {
    usleep(250 * 1000);
  }
}

static void ntp_wait_until_stopped(void) {
  while (rtems_ntpd_running()) {
    usleep(250 * 1000);
  }
}

static void ntp_wait_for_start(void) {
  while (!ntp_start) {
    usleep(250 * 1000);
  }
}

static rtems_task ntpd_runner(
  rtems_task_argument argument
)
{
  while (ntp_run_count++ < 2) {
    char *argv[] = {
      "ntpd",
      "-g",
#if NTP_DEBUG
      "--set-debug-level=" NTP_DEBUG_STR,
#endif
      NULL
    };
    const int argc = ((sizeof(argv) / sizeof(argv[0])) - 1);
    int r;
    ntp_wait_for_start();
    printf("ntpd starting\n");
    r = rtems_ntpd_run(argc, argv);
    printf("ntpd finished: %d\n", r);
    if (r != 0) {
      break;
    }
  }
  printf("error: ntpd: task loop exiting");
  rtems_task_delete(RTEMS_SELF);
}

static void run_test(void)
{
  rtems_status_code sc;
  char *argv[] = {
    "ntpq",
    "127.0.0.1",
    NULL
  };
  const int argc = ((sizeof(argv) / sizeof(argv[0])) - 1);
  int restart_secs = 0;

  setup_etc();

  rtems_shell_add_cmd_struct(&rtems_shell_NTPQ_Command);
  rtems_shell_add_cmd_struct(&rtems_shell_NTPSV_Command);

  sc = rtems_telnetd_start( &rtems_telnetd_config );
  rtems_test_assert( sc == RTEMS_SUCCESSFUL );

  debugger_start();
  debugger_break();

  sc = rtems_shell_init("SHLL", 32 * 1024, 1, CONSOLE_DEVICE_NAME,
    false, false, NULL);
  directive_failed( sc, "rtems_shell_init" );
  assert(sc == RTEMS_SUCCESSFUL);

  sc = rtems_task_create(
    rtems_build_name( 'n', 't', 'p', 'd' ),
    10,
    64 * 1024,
    RTEMS_TIMESLICE,
    RTEMS_FLOATING_POINT,
    &ntpd_id
  );
  directive_failed( sc, "rtems_task_create" );
  sc = rtems_task_start( ntpd_id, ntpd_runner, 0 );
  directive_failed( sc, "rtems_task_start of TA1" );

  ntp_start = true;
  ntp_wait_until_running();

  while (rtems_ntpd_running()) {
    sleep(2);
    restart_secs += 2;
    if (restart_secs == 10) {
      printf("ntpd forced stop\n");
      rtems_ntpd_stop();
      ntp_wait_until_stopped();
      ntp_start = false;
      ntp_wait_until_running();
    }
  }
  printf("ntpd: not running!\n");
}

static rtems_task Init( rtems_task_argument argument )
{
  rtems_printer test_printer;
  rtems_print_printer_printf(&test_printer);
  rtems_test_printer = test_printer;

  TEST_BEGIN();
  fflush(stdout);
  sleep(1);

  rtems_test_assert( net_start() == 0 );

  rtems_shell_init_environment();

  run_test();

  TEST_END();
  fflush(stdout);

  rtems_test_exit( 0 );
}

#define CONFIGURE_MICROSECONDS_PER_TICK 1000

#define CONFIGURE_SHELL_COMMANDS_INIT
#define CONFIGURE_SHELL_COMMANDS_ALL

#include <bsp/irq-info.h>

#define CONFIGURE_SHELL_USER_COMMANDS \
  CONFIGURE_SHELL_USER_COMMANDS_ADAPTER, \
  &rtems_shell_DATE_Command, \
  &rtems_shell_SHUTDOWN_Command

#define CONFIGURE_SHELL_COMMAND_CPUINFO
#define CONFIGURE_SHELL_COMMAND_CPUUSE
#define CONFIGURE_SHELL_COMMAND_PERIODUSE
#define CONFIGURE_SHELL_COMMAND_STACKUSE
#define CONFIGURE_SHELL_COMMAND_PROFREPORT
#define CONFIGURE_SHELL_COMMAND_RTC
#if RTEMS_NET_LEGACY
  #define RTEMS_NETWORKING 1
  #define CONFIGURE_SHELL_COMMANDS_ALL_NETWORKING
#endif /* RTEMS_NET_LEGACY */

#include <rtems/shellconfig.h>

#define CONFIGURE_INIT
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK
#define CONFIGURE_APPLICATION_NEEDS_STUB_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_ZERO_DRIVER

#define CONFIGURE_MAXIMUM_DRIVERS 32
#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 64

#define CONFIGURE_MAXIMUM_USER_EXTENSIONS 1

#define CONFIGURE_UNLIMITED_ALLOCATION_SIZE 32

#define CONFIGURE_BDBUF_BUFFER_MAX_SIZE (64 * 1024)
#define CONFIGURE_BDBUF_MAX_READ_AHEAD_BLOCKS 4
#define CONFIGURE_BDBUF_CACHE_MEMORY_SIZE (1 * 1024 * 1024)

#define CONFIGURE_INIT_TASK_STACK_SIZE (32 * 1024)

#define CONFIGURE_MAXIMUM_TASKS 25

#define CONFIGURE_INITIAL_EXTENSIONS RTEMS_TEST_INITIAL_EXTENSION

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_FLOATING_POINT

#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS

#define CONFIGURE_STACK_CHECKER_ENABLED

#include <rtems/confdefs.h>
