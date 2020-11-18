/*
 * Copyright (c) 2020 Jason Roy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <device.h>
#include <errno.h>
#include <sys/util.h>
#include <sys/__assert.h>

/* The API a hash driver should implement */
__subsystem struct hash_driver_api {

};

void hash_something();
