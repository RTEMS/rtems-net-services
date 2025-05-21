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

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <rtems/ntpd.h>

static const char etc_ntp_conf[] =
  "tos minclock 3 maxclock 6\n"
  "restrict default limited kod nomodify notrap noquery nopeer\n"
  "restrict source  limited kod nomodify notrap noquery\n"
  "restrict 10.0.0.0 mask 255.0.0.0\n"
  "restrict 172.16.0.0 mask 255.240.0.0\n"
  "restrict 192.168.0.0 mask 255.255.0.0\n"
  "restrict 127.0.0.1\n"
  "restrict ::1\n"
  "leapfile \"/etc/leap-seconds\"\n";

/*
 * Leap second default fallback file.
 */
static const char etc_leap_seconds[] =
    "#       Updated through IERS Bulletin C64\n"
    "#       File expires on:  28 June 2023\n"
    "#\n"
    "#@      3896899200\n"
    "#\n"
    "2272060800      10      # 1 Jan 1972\n"
    "2287785600      11      # 1 Jul 1972\n"
    "2303683200      12      # 1 Jan 1973\n"
    "2335219200      13      # 1 Jan 1974\n"
    "2366755200      14      # 1 Jan 1975\n"
    "2398291200      15      # 1 Jan 1976\n"
    "2429913600      16      # 1 Jan 1977\n"
    "2461449600      17      # 1 Jan 1978\n"
    "2492985600      18      # 1 Jan 1979\n"
    "2524521600      19      # 1 Jan 1980\n"
    "2571782400      20      # 1 Jul 1981\n"
    "2603318400      21      # 1 Jul 1982\n"
    "2634854400      22      # 1 Jul 1983\n"
    "2698012800      23      # 1 Jul 1985\n"
    "2776982400      24      # 1 Jan 1988\n"
    "2840140800      25      # 1 Jan 1990\n"
    "2871676800      26      # 1 Jan 1991\n"
    "2918937600      27      # 1 Jul 1992\n"
    "2950473600      28      # 1 Jul 1993\n"
    "2982009600      29      # 1 Jul 1994\n"
    "3029443200      30      # 1 Jan 1996\n"
    "3076704000      31      # 1 Jul 1997\n"
    "3124137600      32      # 1 Jan 1999\n"
    "3345062400      33      # 1 Jan 2006\n"
    "3439756800      34      # 1 Jan 2009\n"
    "3550089600      35      # 1 Jul 2012\n"
    "3644697600      36      # 1 Jul 2015\n"
    "3692217600      37      # 1 Jan 2017\n";

static const char etc_services[] =
    "ntp                123/tcp      # Network Time Protocol  [Dave_Mills] [RFC5905]\n"
    "ntp                123/udp      # Network Time Protocol  [Dave_Mills] [RFC5905]\n";

static int make_etc_path(void)
{
  const char* etc = "/etc";
  struct stat sb;
  int r;
  r = stat(etc, &sb);
  if (r == 0) {
    if (!S_ISDIR(sb.st_mode)) {
      printf("ntp: /etc exists and not a directory\n");
      return -1;
    }
    return 0;
  }
  if (r < 0 && errno != ENOENT) {
    printf(
      "ntp: /etc stat failed: %d %s\n", errno, strerror(errno));
    return -1;
  }
  r = mkdir(etc, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if (r != 0) {
    printf(
      "ntp: /etc mkdir failed: %d %s\n", errno, strerror(errno));
    return -1;
  }
  return 0;
}

static int copy_text_to_file(
  const char* dst, const char* text, const size_t len, int append)
{
  const char* mode = "w";
  FILE* fp;
  size_t w;
  int r = 0;
  if (append) {
    mode = "a";
  }
  fp = fopen(dst, mode);
  if (fp == NULL) {
    printf(
      "ntp: file open error: %s: %d %s\n", dst, errno, strerror(errno));
    return -1;
  }
  w = fwrite(text, len, 1, fp);
  if (w != 1) {
    printf(
      "ntp: file write error: %s: %d %s\n", dst, errno, strerror(errno));
    r = -1;
  }
  fclose(fp);
  return r;
}

int rtems_ntpd_client_pool_config(const char* pool_ip) {
  int r = make_etc_path();
  if (r == 0) {
    char buf[128];
    int len = snprintf(buf, sizeof(buf), "pool %s iburst\n", pool_ip);
    r = copy_text_to_file("/etc/ntp.conf", buf, len, 0);
    if (r == 0) {
      r = copy_text_to_file(
        "/etc/ntp.conf", etc_ntp_conf, sizeof(etc_ntp_conf), 1);
      if (r == 0) {
        r = copy_text_to_file(
          "/etc/leap-seconds", etc_leap_seconds, sizeof(etc_leap_seconds), 1);
      }
    }
  }
  return r;
}

int rtems_ntpd_add_etc_services(void) {
  int r = make_etc_path();
  if (r == 0) {
    r = copy_text_to_file(
      "/etc/services", etc_services, sizeof(etc_services), 1);
  }
  return r;
}
