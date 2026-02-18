/****************************************************************************
 * boards/arm/imx9/imx93-qsb/src/imx93_bringup.c
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2024 NXP
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/fs/fs.h>
#include <sys/types.h>
#include <syslog.h>
#include "imx93-qsb.h"

#include <fcntl.h>      // For O_RDWR
#include <unistd.h>     // For sleep, dup2, close
#include <nuttx/board.h>

#include <stdio.h>

#ifdef CONFIG_RPTUN
#  include <imx9_rptun.h>
#endif

#ifdef CONFIG_NSH_LIBRARY
extern int nsh_consolemain(int argc, FAR char *argv[]);
#endif

#ifdef CONFIG_RPMSG_UART_RAW
#  include <nuttx/serial/uart_rpmsg_raw.h>
#endif

#ifdef CONFIG_IMX9_FLEXCAN
#  include "imx9_flexcan.h"
#endif


/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_RPMSG_UART_RAW
static int nsh_rpmsg_task(int argc, FAR char *argv[])
{
  int fd;
  int ret;

  sleep(1);

  fd = open("/dev/tty-nsh", O_RDWR);

  if (fd < 0)
    {
      syslog(LOG_ERR, "Failed to open /dev/tty-nsh: %d\n", errno);
      return -1;
    }
  
  dup2(fd, 0);
  dup2(fd, 1);
  dup2(fd, 2);
  
  close(fd);
  
  ret = nsh_consolemain(0, NULL);
  
  return ret;
}
#endif


/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_RPMSG_UART_RAW
void rpmsg_serialrawinit(void)
{
  uart_rpmsg_raw_init("netcore", "-nsh", 4096, true);
  uart_rpmsg_raw_init("netcore", "-test", 4096, false);
}
#endif


/****************************************************************************
 * Name: imx_bringup
 *
 * Description:
 *   Bring up board features
 *
 ****************************************************************************/

int imx93_bringup(void)
{
  int ret;
  syslog(LOG_INFO, "imx93 bringup\n");

#ifdef CONFIG_RPTUN
  imx9_rptun_init("imx9-shmem", "netcore");
#endif

#ifdef CONFIG_FS_PROCFS
  /* Mount the procfs file system */

  ret = nx_mount(NULL, "/proc", "procfs", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to mount procfs at /proc: %d\n", ret);
    }
#endif

#if defined(CONFIG_IMX9_LPI2C)
  /* Configure I2C peripheral interfaces */

  ret = imx93_i2c_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "Failed to initialize I2C driver: %d\n", ret);
    }
#endif

#if defined(CONFIG_IMX9_LPSPI1)
  /* Configure SPI peripheral interfaces */

  ret = imx93_spi_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "Failed to initialize SPI driver: %d\n", ret);
    }
#endif

#ifdef CONFIG_IMX9_FLEXCAN1
  imx9_caninitialize(1);
#endif

#ifdef CONFIG_IMX9_FLEXCAN2
  imx9_caninitialize(2);
#endif

#ifdef CONFIG_RPMSG_UART_RAW
  // Spawn NSH on rpmsg
  task_create("nsh_rpmsg",
              CONFIG_SYSTEM_NSH_PRIORITY,
              CONFIG_SYSTEM_NSH_STACKSIZE,
              nsh_rpmsg_task,
              NULL);
#endif

  UNUSED(ret);
  return OK;
}
