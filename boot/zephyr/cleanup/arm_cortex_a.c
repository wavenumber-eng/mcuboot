/*
 * Copyright (c) 2026 Wavenumber
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Cortex-A cleanup before chain-loading the application.
 *
 * The interrupt half is the same shape as arm_cortex_r.c. What an A needs and
 * an R does not is below it: an MMU rather than an MPU, and a branch predictor
 * that has to be invalidated because Zephyr's AArch32 cache code does not.
 *
 * An external L2 controller is deliberately not handled here. It is not part of
 * the core, SoCs integrate it differently, and some Cortex-A parts have none,
 * so it belongs to the SoC layer. Where one is enabled the SoC must clean it
 * before this runs, or the application reads stale memory.
 */

#include <stdint.h>
#include <zephyr/irq.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#ifdef CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER
extern void z_soc_irq_eoi(unsigned int irq);
#else
#include <zephyr/drivers/interrupt_controller/gic.h>
#endif

#define WRITE_CP15(value, coproc, opc1, crn, crm, opc2)                        \
	__asm__ volatile("mcr " #coproc ", " #opc1 ", %0, " #crn ", " #crm ", " \
			 #opc2 "\n" ::"r"(value) :"memory")

#define READ_CP15(out, coproc, opc1, crn, crm, opc2)                           \
	__asm__ volatile("mrc " #coproc ", " #opc1 ", %0, " #crn ", " #crm ", " \
			 #opc2 "\n" : "=r"(out)::"memory")

/* System control register bits this file clears. */
#define SCTLR_M BIT(0)  /* MMU */
#define SCTLR_C BIT(2)  /* data cache */
#define SCTLR_I BIT(12) /* instruction cache */
#define SCTLR_Z BIT(11) /* branch prediction */

void cleanup_arm_interrupts(void)
{
	/* Allow any pending interrupts to be recognized */
	__ISB();
	__disable_irq();

	for (unsigned int i = 0; i < CONFIG_NUM_IRQS; ++i) {
		irq_disable(i);
	}

	for (unsigned int i = 0; i < CONFIG_NUM_IRQS; ++i) {
#ifdef CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER
		z_soc_irq_eoi(i);
#else
		arm_gic_eoi(i);
#endif /* CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */
	}
}

#if defined(CONFIG_ARM_AARCH32_MMU)
__weak void z_arm_clear_arm_mmu_config(void)
{
	uint32_t sctlr;

	/*
	 * Runs after the caches have been cleaned and disabled, so the image
	 * already sits in memory. What is left is the state that would still
	 * describe this boot loader once the application is running.
	 *
	 * The instruction cache and the branch predictor are invalidated rather
	 * than only disabled: entries fetched and predictions made against these
	 * translation tables are not valid under the ones the application
	 * installs, and arch_icache_disable() clears the enable bit without
	 * invalidating anything.
	 */
	WRITE_CP15(0, p15, 0, c7, c5, 0); /* invalidate the instruction cache */
	WRITE_CP15(0, p15, 0, c7, c5, 6); /* invalidate the branch predictor */
	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	READ_CP15(sctlr, p15, 0, c1, c0, 0);
	sctlr &= ~(SCTLR_M | SCTLR_C | SCTLR_I | SCTLR_Z);
	barrier_dsync_fence_full();
	WRITE_CP15(sctlr, p15, 0, c1, c0, 0);
	barrier_isync_fence_full();

	/*
	 * The translation lookaside buffer is invalidated after the unit is
	 * switched off, so nothing can refill it from tables that are about to
	 * stop being the ones in use.
	 */
	WRITE_CP15(0, p15, 0, c8, c7, 0); /* invalidate the entire unified TLB */
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}
#endif /* CONFIG_ARM_AARCH32_MMU */
