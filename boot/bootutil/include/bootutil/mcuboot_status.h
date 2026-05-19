/*
 *  Copyright (c) 2022, Laird Connectivity
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MCUBOOT_STATUS_
#define H_MCUBOOT_STATUS_

#include <stdint.h>

/* Enumeration representing the states that MCUboot can be in */
typedef enum
{
	MCUBOOT_STATUS_STARTUP = 0,
	MCUBOOT_STATUS_UPGRADING,
	MCUBOOT_STATUS_BOOTABLE_IMAGE_FOUND,
	MCUBOOT_STATUS_NO_BOOTABLE_IMAGE_FOUND,
	MCUBOOT_STATUS_BOOT_FAILED,
	MCUBOOT_STATUS_USB_DFU_WAITING,
	MCUBOOT_STATUS_USB_DFU_ENTERED,
	MCUBOOT_STATUS_USB_DFU_TIMED_OUT,
	MCUBOOT_STATUS_SERIAL_DFU_ENTERED,
} mcuboot_status_type_t;

typedef enum
{
	MCUBOOT_STATUS_PROGRESS_PREPARE = 0,
	MCUBOOT_STATUS_PROGRESS_VALIDATE,
	MCUBOOT_STATUS_PROGRESS_SWAP,
	MCUBOOT_STATUS_PROGRESS_DONE,
} mcuboot_status_progress_operation_t;

struct mcuboot_status_progress {
	mcuboot_status_type_t status;
	mcuboot_status_progress_operation_t operation;
	uint8_t image_index;
	uint8_t reserved[3];
	uint32_t current;
	uint32_t total;
	uint8_t version_major;
	uint8_t version_minor;
	uint16_t version_revision;
	uint32_t version_build;
	const char *message;
};

#if defined(CONFIG_MCUBOOT_ACTION_HOOKS)
extern void mcuboot_status_change(mcuboot_status_type_t status);
#else
#define mcuboot_status_change(_status) do {} while (0)
#endif

#if defined(CONFIG_MCUBOOT_PROGRESS_HOOKS)
extern void mcuboot_status_progress(const struct mcuboot_status_progress *progress);
#else
#define mcuboot_status_progress(_progress) do {} while (0)
#endif

#endif /* H_MCUBOOT_STATUS_ */
