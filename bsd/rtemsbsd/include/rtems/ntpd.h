/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @ingroup rtems_bsd
 *
 * @brief This header file defines the NTP daemon interfaces.
 */

/*
 * Copyright (C) 2022 embedded brains GmbH (http://www.embedded-brains.de)
 * Copyright (C) 2025 Contemporary Software (chris@contemporary.software)
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

#ifndef _RTEMS_NTPD_H
#define _RTEMS_NTPD_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System Variables, the data you get from `ntpq -c rl`
 */
typedef struct {
  unsigned int status;
  char status_str[128];
  char version[32];
  char processor[32];
  char system[32];
  uint8_t leap;
  uint8_t stratum;
  int8_t precision;
  double rootdelay;
  double rootdisp;
  char refid[32];
  uint64_t reftime_sec;
  uint64_t reftime_nsec;
  uint64_t clock_sec;
  uint64_t clock_nsec;
  int peer;
  int tc;                  /* ntpq: poll */
  int mintc;               /* ntpq: minpoll */
  double offset;
  double frequency;        /* ntpd: drift */
  double sys_jitter;
  double clk_jitter;       /* ntpd: error */
  double clk_wander;       /* ntpd: clock_stability */
  int tai;
  uint64_t leapsec;        /* ntpd: leaptab */
  uint64_t expire;         /* ntpd: leapend */
} ntp_sys_var_data;

/**
 * @brief Runs the NTP daemon (nptd).
 *
 * It is recommended to use the ``-g`` option.  The NTP daemon will start
 * worker theads which inherit scheduler attributes from the runner thread.
 *
 * @param argc is the argument count.
 *
 * @param argv is the vector of arguments.
 *
 * @return This function only returns if @ref rtems_ntpd_stop is
 *   called or the daemon is already running  Any other reason is a
 *   serious error.
 */
int rtems_ntpd_run(int argc, char **argv);

/**
 * @brief Stops the NTP daemon (nptd).
 *
 * The ntpd loop will exit when it next runs cleaning up. Use the
 * @ref rtems_ntpd_running call to check if the daemon has stopped
 * running.
 *
 * @return This function should never return.  If it returns, then there is a
 *   serious error.
 */
void rtems_ntpd_stop(void);

/**
 * @brief Checks if the NTP daemon (nptd) is running?
 *
 * @return Return 1 if ntpd is running else 0 is returned.
 */
int rtems_ntpd_running(void);

/**
 * @brief Get the system variable data
 */
void rtems_ntpd_get_sys_vars(ntp_sys_var_data* sv);

/**
 * @brief Is the NTP synchronized to a clock source? Returns 1 or true
 *        if synchronized.
 */
int rtems_ntpd_is_synchronized(ntp_sys_var_data* sv);

/**
 * @brief Is there a leap warning? Returns 1 or true if an alarm.
 */
int rtems_ntpd_leap_warning(ntp_sys_var_data* sv);

/**
 * @brief Is there a leap alarm? Returns 1 or true if an alarm.
 */
int rtems_ntpd_leap_alarm(ntp_sys_var_data* sv);

/**
 * @brief Lock the NTPD lock?
 */
void rtems_ntpd_lock(void);

/**
 * @brief Unlock the NTPD lock?
 */
void rtems_ntpd_unlock(void);

#ifdef __cplusplus
}
#endif

#endif /* _RTEMS_NTPD_H */
