/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __BASEBOARD_VARIANTS_H__
#define __BASEBOARD_VARIANTS_H__

#include <chip.h>
#include <soc/gpio.h>
#include <soc/meminit.h>
#include <stdint.h>

/* The next set of functions return the gpio table and fill in the number of entries for
 * each table.
 */

const struct pad_config *variant_gpio_table(size_t *num);
const struct pad_config *variant_gpio_override_table(size_t *num);
const struct pad_config *variant_early_gpio_table(size_t *num);
const struct cros_gpio *variant_cros_gpios(size_t *num);

const struct mb_cfg *variant_memory_params(void);
int variant_memory_sku(void);
bool variant_is_half_populated(void);
void variant_update_soc_chip_config(struct soc_intel_alderlake_config *config);

#endif /*__BASEBOARD_VARIANTS_H__ */
