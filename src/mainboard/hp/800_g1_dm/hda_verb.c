/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/azalia_device.h>

const u32 cim_verb_data[] = {
	0x80862807,	/* Codec Vendor / Device ID: Intel */
	0x80860101,	/* Subsystem ID */
	4,		/* Number of 4 dword sets */
	AZALIA_SUBVENDOR(0, 0x80860101),
	AZALIA_PIN_CFG(0, 0x05, 0x18560010),
	AZALIA_PIN_CFG(0, 0x06, 0x18560020),
	AZALIA_PIN_CFG(0, 0x07, 0x18560030),
};

const u32 pc_beep_verbs[0] = {};

AZALIA_ARRAY_SIZES;
