/*
 * Copyright (c) 2014 TRUSTONIC LIMITED
 * All rights reserved
 *
 * The present software is the confidential and proprietary information of
 * TRUSTONIC LIMITED. You shall not disclose the present software and shall
 * use it only in accordance with the terms of the license agreement you
 * entered into with TRUSTONIC LIMITED. This software may be subject to
 * export or import laws in certain countries.
 */

#include <assert.h>
#include <debug.h>
#include <string.h>
#include <stdio.h>
#include <platform.h>
#include <mmio.h>
#include <platform_def.h>
#include <console.h>
#include "plat_private.h"
#include "tkcore.h"

#include <arch_helpers.h>

#include <gic_v2.h>
#include <gic_v3.h>
#include <interrupt_mgmt.h>

int gic_populate_rdist(unsigned int *rdist_base);
void irq_raise_softirq(unsigned int map, unsigned int irq);

static void tkcore_platform_dump(void);

extern void aee_wdt_dump();
/* Code */
uint32_t plat_tkcore_secure_service_request(uint64_t service_id, uint64_t *x2, uint64_t *x3)
{
	uint32_t rc = PLAT_TKCORE_SECURE_SERVICE_SUCCESS;
	uint32_t linear_id = platform_get_core_pos(read_mpidr());

	(void) x2;
	(void) x3;
	switch (service_id) {
		case PLAT_TKCORE_SECURE_SERVICE_TEST:
			console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);
			tf_printf("%s core 0x%x service PLAT_TKCORE_SECURE_SERVICE_TEST\n",
				__func__, (int) linear_id);
			console_uninit();
			break;
		case PLAT_TKCORE_SECURE_SERVICE_WDT_DUMP:
			tkcore_platform_dump();
			break;
		default:
			rc = PLAT_TKCORE_SECURE_SERVICE_ERROR;
	}

	return rc;
}

void plat_tkcore_dump(void)
{
	aee_wdt_dump();
}

static void tkcore_platform_dump(void)
{
	uint64_t mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(mpidr);

	uint32_t SGITargets;
	uint32_t rdist_sgi_base = 0;

	/* get the base of redistributor first */
	if (gic_populate_rdist(&rdist_sgi_base) == -1) {
		return;
	}
	/* Configure SGI */
	gicd_clr_igroupr(rdist_sgi_base, FIQ_SMP_CALL_SGI);
	gicd_set_ipriorityr(rdist_sgi_base, FIQ_SMP_CALL_SGI, GIC_HIGHEST_SEC_PRIORITY);

	/* Enable SGI */
	gicd_set_isenabler(rdist_sgi_base, FIQ_SMP_CALL_SGI);

	/* Send SGIs to all cores except the current one
	   (current will directly branch to the dump handler) */
	SGITargets = 0xFF;
	SGITargets &= ~(1 << linear_id);

	/* Trigger SGI */
	irq_raise_softirq(SGITargets, FIQ_SMP_CALL_SGI);

	/* Current core directly branches to dump handler */
	plat_tkcore_dump();
}

