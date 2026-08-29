/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-10-28     zhangyang     first version
 */

#ifndef __AT_DEVICE_NL668_H__
#define __AT_DEVICE_NL668_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <at_device.h>

/* The maximum number of sockets supported by the nl668 device */
#define AT_DEVICE_NL668_SOCKETS_NUM    6

struct at_device_nl668
{
    char *device_name;
    char *client_name;

    int power_pin;
    int power_status_pin;
    size_t recv_line_num;
    struct at_device device;

    void *socket_data;
    void *user_data;

    rt_bool_t power_status;
    rt_bool_t sleep_status;
};

#ifdef AT_USING_SOCKET

/* nl668 device socket initialize */
int nl668_socket_init(struct at_device *device);

/* nl668 device class socket register */
int nl668_socket_class_register(struct at_device_class *class);

#endif /* AT_USING_SOCKET */

#ifdef __cplusplus
}
#endif

#endif /* __AT_DEVICE_NL668_H__ */
