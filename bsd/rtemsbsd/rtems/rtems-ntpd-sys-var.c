/**
 * @file
 *
 * @ingroup rtems_bsd_rtems
 *
 * @brief NTP System Variables
 */

/*
 * Copyright (c) 2025 Chris Johns <chrisj@rtems.org>.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>

#include <rtems/ntpd.h>

#include <rtems/shellconfig-net-services.h>

int rtems_shell_ntpsv_command(int argc, char **argv) {
  const int column = 12;
  ntp_sys_var_data sv;
  rtems_ntpd_get_sys_vars(&sv);
  printf(
    "%*s: %s\n", column, "Synchronized",
    rtems_ntpd_is_synchronized(&sv) ? "yes" : "no");
  printf(
    "%*s: %s\n", column, "Leap warning",
    rtems_ntpd_leap_warning(&sv) ? "yes" : "no");
  printf(
    "%*s: %s\n", column, "Leap alarm",
    rtems_ntpd_leap_alarm(&sv) ? "yes" : "no");
  printf("%*s: %s (%04x)\n", column, "status", sv.status_str, sv.status);
  printf("%*s: %s\n", column, "version", sv.version);
  printf("%*s: %s\n", column, "processor", sv.processor);
  printf("%*s: %s\n", column, "system", sv.system);
  printf("%*s: %u\n", column, "leap", (unsigned int) sv.leap);
  printf("%*s: %u\n", column, "stratum", (unsigned int) sv.stratum);
  printf("%*s: %i\n", column, "precision", (int) sv.precision);
  printf("%*s: %f\n", column, "rootdelay", sv.rootdelay);
  printf("%*s: %f\n", column, "rootdisp", sv.rootdisp);
  printf("%*s: %s\n", column, "refid", sv.refid);
  printf("%*s: %llx.%llx\n", column, "reftime", sv.reftime_sec, sv.reftime_nsec);
  printf("%*s: %llx.%llx\n", column, "clock", sv.clock_sec, sv.clock_nsec);
  printf("%*s: %i\n", column, "peer", sv.peer);
  printf("%*s: %i\n", column, "tc", sv.tc);
  printf("%*s: %i\n", column, "mintc", sv.mintc);
  printf("%*s: %f\n", column, "offset", sv.offset);
  printf("%*s: %f\n", column, "frequency", sv.frequency);
  printf("%*s: %f\n", column, "sys_jitter", sv.sys_jitter);
  printf("%*s: %f\n", column, "clk_jitter", sv.clk_jitter);
  printf("%*s: %f\n", column, "clk_wander", sv.clk_wander);
  printf("%*s: %i\n", column, "tai", sv.tai);
  printf("%*s: %llu\n", column, "leapsec", sv.leapsec);
  printf("%*s: %llu\n", column, "expire", sv.expire);
  return 0;
}

rtems_shell_cmd_t rtems_shell_NTPSV_Command =
{
    "ntpsv",
    "[help]",
    "misc",
    rtems_shell_ntpsv_command,
    NULL,
    NULL
};
