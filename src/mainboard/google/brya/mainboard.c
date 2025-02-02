/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <acpi/acpi.h>
#include <baseboard/variants.h>
#include <device/device.h>
#include <ec/ec.h>
#include <soc/ramstage.h>
#include <vendorcode/google/chromeos/chromeos.h>

void mainboard_update_soc_chip_config(struct soc_intel_alderlake_config *config)
{
	variant_update_soc_chip_config(config);
}

__weak void variant_update_soc_chip_config(struct soc_intel_alderlake_config *config)
{
	/* default implementation does nothing */
}

static void mainboard_init(void *chip_info)
{
	const struct pad_config *base_pads;
	const struct pad_config *override_pads;
	size_t base_num, override_num;

	base_pads = variant_gpio_table(&base_num);
	override_pads = variant_gpio_override_table(&override_num);
	gpio_configure_pads_with_override(base_pads, base_num, override_pads, override_num);
}

static void mainboard_dev_init(struct device *dev)
{
	mainboard_ec_init();
}

static void mainboard_enable(struct device *dev)
{
	dev->ops->init = mainboard_dev_init;
	dev->ops->acpi_inject_dsdt = chromeos_dsdt_generator;
}

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
	.enable_dev = mainboard_enable,
};
