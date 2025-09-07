/*
 * calibrate.c: Sony PS4 TSC/LAPIC calibration
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#define pr_fmt(fmt) "ps4: " fmt

#include <linux/jiffies.h>
#include <asm/io.h>
#include <asm/msr.h>
#include <asm/ps4.h>
#include <asm/delay.h>
#include <asm/apic.h>
#include <linux/io.h>

/* The PS4 southbridge (Aeolia) has an EMC timer that ticks at 32.768kHz,
 * which seems to be an appropriate clock reference for calibration. Both TSC
 * and the LAPIC timer are based on the core clock frequency and thus can be
 * calibrated together. */
static void __iomem *emc_timer = NULL;

static __init inline u32 emctimer_read32(unsigned int reg)
{
	return ioread32(emc_timer + reg);
}

static __init inline void emctimer_write32(unsigned int reg, u32 val)
{
	iowrite32(val, emc_timer + reg);
}

static __init inline u32 emctimer_read(void)
{
	u32 t1, t2;
	t1 = emctimer_read32(EMC_TIMER_VALUE);
	while (1) {
		t2 = emctimer_read32(EMC_TIMER_VALUE);
		if (t1 == t2)
			return t1;
		t1 = t2;
	}
}

static __init unsigned long ps4_measure_tsc_freq(void)
{
    unsigned long ret;

    ret = 2500000000UL; // GHz 1594000000UL

    pr_info("ps4: Forzando TSC frequency a %ld Hz\n", ret);

    return ret;
}

unsigned long __init ps4_calibrate_tsc(void)
{
	unsigned long tsc_freq = ps4_measure_tsc_freq();

	if (!tsc_freq) {
		pr_warn("Unable to measure TSC frequency, assuming default.\n");
		tsc_freq = PS4_DEFAULT_TSC_FREQ;
	}

	lapic_timer_period = (tsc_freq + 8 * HZ) / (16 * HZ);

	return (tsc_freq + 500) / 1000;
}
