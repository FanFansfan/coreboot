/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/pnp_ops.h>
#include <superio/nuvoton/npcd378/npcd378.h>
#include <superio/nuvoton/common/nuvoton.h>
#include <southbridge/intel/lynxpoint/pch.h>


#define SERIAL_DEV PNP_DEV(0x2e, NPCD378_SP2)

void mainboard_config_superio(void)
{
	/* Enable UART */
	if (CONFIG(CONSOLE_SERIAL))
		nuvoton_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
}
