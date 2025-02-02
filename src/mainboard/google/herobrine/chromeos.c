/* SPDX-License-Identifier: GPL-2.0-only */

#include <boot/coreboot_tables.h>
#include <bootmode.h>
#include "board.h"

void setup_chromeos_gpios(void)
{
	gpio_input_pullup(GPIO_SD_CD_L);
}

void fill_lb_gpios(struct lb_gpios *gpios)
{
	struct lb_gpio chromeos_gpios[] = {
		{GPIO_SD_CD_L.addr, ACTIVE_LOW, gpio_get(GPIO_SD_CD_L),
			"SD card detect"},
	};

	lb_add_gpios(gpios, chromeos_gpios, ARRAY_SIZE(chromeos_gpios));
}
