#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <bl31.h>
#include <context_mgmt.h>
#include <debug.h>
#include <errno.h>
#include <platform.h>
#include <runtime_svc.h>
#include <stddef.h>
#include <console.h>
#include <tkcore.h>
#include <uuid.h>
#include "plat_private.h"
#include "tkcored_private.h"
#include "tkcore.h"
#include "teesmc.h"
#include "teesmc_tkcored.h"

#if (defined(MACH_TYPE_MT6735) || defined(MACH_TYPE_MT6735M) || \
	defined(MACH_TYPE_MT6753) || defined(MACH_TYPE_MT8173))
#define TKCORE_BOOT_ARG_ADDR (((struct atf_arg_t *)(uintptr_t) TEE_BOOT_INFO_ADDR)->tee_boot_arg_addr)
#else
#define TKCORE_BOOT_ARG_ADDR (((struct atf_arg_t *)(uintptr_t) (&gteearg))->tee_boot_arg_addr)
#endif

tkcore_vectors_t *tkcore_vectors;

tkcore_context_t tkcored_sp_context[TKCORED_CORE_COUNT];
uint32_t tkcored_rw;

//static uint64_t tkcoreBaseCoreMpidr;

static int32_t tkcored_init(void);

static void dump_caller_state(void *handle)
{
	uint64_t elr_el3, spsr_el3, sp_el1;

	elr_el3 = SMC_GET_EL3(handle, CTX_ELR_EL3);
	spsr_el3 = SMC_GET_EL3(handle, CTX_SPSR_EL3);
	sp_el1 = read_ctx_reg(get_sysregs_ctx(handle), CTX_SP_EL1);

	tf_printf("ELR_EL3: 0x%016lx\tSPSR_EL3: 0x%016lx\tSP_EL1: 0x%016lx\n",
		elr_el3, spsr_el3, sp_el1);

	tf_printf("X0: 0x%016lx\tX1: 0x%016lx\tX2: 0x%016lx\tX3: 0x%016lx\n",
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X0),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X1),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X2),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X3));

	tf_printf("X4: 0x%016lx\tX5: 0x%016lx\tX6: 0x%016lx\tX7: 0x%016lx\n",
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X4),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X5),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X6),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X7));

	tf_printf("X8: 0x%016lx\tX9: 0x%016lx\tX10: 0x%016lx\tX11: 0x%016lx\n",
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X8),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X9),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X10),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X11));

	tf_printf("X12: 0x%016lx\tX13: 0x%016lx\tX14: 0x%016lx\tX15: 0x%016lx\n",
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X12),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X13),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X14),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X15));

	tf_printf("X16: 0x%016lx\tX17: 0x%016lx\tX18: 0x%016lx\tX19: 0x%016lx\n",
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X16),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X17),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X18),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X19));

	tf_printf("X20: 0x%016lx\tX21: 0x%016lx\tX22: 0x%016lx\tX23: 0x%016lx\n",
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X20),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X21),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X22),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X23));

	tf_printf("X24: 0x%016lx\tX25: 0x%016lx\tX26: 0x%016lx\tX27: 0x%016lx\n",
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X24),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X25),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X26),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X27));

	tf_printf("X28: 0x%016lx\tX29: 0x%016lx\tLR: 0x%016lx\tSP_EL0: 0x%016lx\n",
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X28),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X29),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_LR),
		read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_SP_EL0));
}

static uint64_t tkcored_sel1_interrupt_handler(uint32_t id,
						uint32_t flags,
						void *handle,
						void *cookie)
{
	unsigned int linear_id;
	tkcore_context_t *tkcore_ctx;
	uint32_t irq = id & 0x3ff;

	/* Check the security state when the exception was generated
	   FIQ for swd will be directly directed to EL1 for GICv2. */
	assert(get_interrupt_src_ss(flags) == NON_SECURE);

#if IMF_READ_INTERRUPT_ID
	/* Check the security status of the interrupt */
	assert(plat_ic_get_interrupt_type(id) == INTR_TYPE_S_EL1);
#endif

	/* Sanity check the pointer to this cpu's context */
	linear_id = platform_get_core_pos(read_mpidr());
	assert(handle == cm_get_context_by_index(linear_id, NON_SECURE));

	if (irq == FIQ_SMP_CALL_SGI) {

		console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);

		tf_printf("##### %s ACK PLAT_SMP_CALL_SGI. Dump State. ###### \n", __func__);

		plat_ic_acknowledge_interrupt();
		plat_ic_end_of_interrupt(irq);

		plat_tkcore_dump();
	} else if (irq == WDT_IRQ_BIT_ID) {
		if (plat_ic_acknowledge_interrupt() == WDT_IRQ_BIT_ID) {

			console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);
			tf_printf("##### %s ACK WDT_IRQ. Redirecting to TEE... ###### \n", __func__);

			/* which means this core successfully ack WDT IRQ ahead of all other cores.
				and others won't able to consume this irq any more. */
			plat_tkcore_secure_service_request(PLAT_TKCORE_SECURE_SERVICE_WDT_DUMP, NULL, NULL);

			tf_printf("##### %s ACK WDT_IRQ. Returns from TEE... ###### \n", __func__);

			plat_ic_end_of_interrupt(irq);
		} else {

			console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);

			tf_printf("##### %s WDT_IRQ already ACKED. ###### \n", __func__);
			tf_printf("##### %s Return to NS-EL1. ###### \n", __func__);

			SMC_RET0(handle);
		}
	} else {
		console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);

		tf_printf("##### %s UNKNOWN SEL1-IRQ ID %u ##### \n", __func__, irq);
		dump_caller_state(handle);

		/* Other GROUP1-interrupt will be redirected to tkcore */
		/* Currently S-EL1 irq cannot be read by ATF. But we know it's a WDT */
		plat_tkcore_secure_service_request(PLAT_TKCORE_SECURE_SERVICE_WDT_DUMP, NULL, NULL);

		/* Save the non-secure context before entering the TKCORE */
		cm_el1_sysregs_context_save(NON_SECURE);

		/* Get a reference to this cpu's TKCORE context */
		tkcore_ctx = &tkcored_sp_context[linear_id];
		assert(&tkcore_ctx->cpu_ctx == cm_get_context_by_index(linear_id, SECURE));

#if 0
	SMC_SET_EL3(&tkcore_ctx->cpu_ctx,
			CTX_SPSR_EL3,
			SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS));
#endif
		SMC_SET_EL3(&tkcore_ctx->cpu_ctx,
				CTX_ELR_EL3,
				(uint64_t)&tkcore_vectors->fiq_entry);
		cm_el1_sysregs_context_restore(SECURE);
		cm_set_next_eret_context(SECURE);

		SMC_RET2(&tkcore_ctx->cpu_ctx, TKCORE_HANDLE_FIQ_AND_RETURN, read_elr_el3());
	}

	return 0;
}

// Core-specific context initialization for non-primary cores
void tkcored_init_core(uint64_t mpidr) {
#if 0
	uint32_t linear_id = platform_get_core_pos(mpidr);
	tkcore_context_t *tkcore_ctx = &tkcored_sp_context[linear_id];
	uint32_t boot_core_nro;

	tbase_init_secure_context(tbase_ctx);

	boot_core_nro = platform_get_core_pos(tkcoreBootCoreMpidr);
	save_sysregs_core(boot_core_nro, linear_id);
#endif
}

int32_t tkcored_setup(void)
{
	entry_point_info_t *image_info;
	int32_t rc;
	uint64_t mpidr = read_mpidr();
	uint32_t linear_id;

	linear_id = platform_get_core_pos(mpidr);

	/*
	 * Get information about the Secure Payload (BL32) image. Its
	 * absence is a critical failure.  TODO: Add support to
	 * conditionally include the SPD service
	 */
	image_info = bl31_plat_get_next_image_ep_info(SECURE);
	assert(image_info);

	/*
	 * If there's no valid entry point for SP, we return a non-zero value
	 * signalling failure initializing the service. We bail out without
	 * registering any handlers
	 */
	if (!image_info->pc)
		return 1;

	/*
	 * We could inspect the SP image and determine it's execution
	 * state i.e whether AArch32 or AArch64. Assuming it's AArch32
	 * for the time being.
	 */
	tkcored_rw = TKCORE_AARCH32;
	rc = tkcored_init_secure_context(image_info->pc, tkcored_rw,
						mpidr, &tkcored_sp_context[linear_id]);
	assert(rc == 0);

	/*
	 * All TKCORED initialization done. Now register our init function with
	 * BL31 for deferred invocation
	 */
	bl31_register_bl32_init(&tkcored_init);

	return rc;
}

static int32_t tkcored_init(void)
{
	uint64_t mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(mpidr), flags;
	uint64_t rc;
	tkcore_context_t *tkcore_ctx = &tkcored_sp_context[linear_id];

	// Save el1 registers in case non-secure world has already been set up.
	cm_el1_sysregs_context_save(NON_SECURE);

	/*
	 * Arrange for an entry into the test secure payload. We expect an array
	 * of vectors in return
	 */

	write_ctx_reg(get_gpregs_ctx(&(tkcore_ctx->cpu_ctx)), CTX_GPREG_X0,
		(int64_t) TKCORE_BOOT_ARG_ADDR );
	tf_printf("TKCore Entry %016lx\n",
		read_ctx_reg(get_el3state_ctx(&(tkcore_ctx->cpu_ctx)), CTX_ELR_EL3));
	rc = tkcored_synchronous_sp_entry(tkcore_ctx);
	tf_printf("TKCore Exit\n");
	assert(rc != 0);
	if (rc) {
		set_tkcore_pstate(tkcore_ctx->state, TKCORE_PSTATE_ON);

		/*
		 * TKCORE has been successfully initialized. Register power
		 * managemnt hooks with PSCI
		 */
		psci_register_spd_pm_hook(&tkcored_pm);
	}

	/*
	 * Register an interrupt handler for S-EL1 interrupts when generated
	 * during code executing in the non-secure state.
	 */
	flags = 0;
	set_interrupt_rm_flag(flags, NON_SECURE);
	rc = register_interrupt_type_handler(INTR_TYPE_S_EL1,
		tkcored_sel1_interrupt_handler, flags);
	if (rc)
		panic();

	cm_el1_sysregs_context_restore(NON_SECURE);

	return rc;
}

uint64_t tkcored_smc_handler(uint32_t smc_fid,
			uint64_t x1,
			uint64_t x2,
			uint64_t x3,
			uint64_t x4,
			void *cookie,
			void *handle,
			uint64_t flags)
{
	cpu_context_t *ns_cpu_context;
	unsigned long mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(mpidr);
	tkcore_context_t *tkcore_ctx = &tkcored_sp_context[linear_id];

	/*
	 * Determine which security state this SMC originated from
	 */

	if (is_caller_non_secure(flags)) {
		assert(handle == cm_get_context_by_index(linear_id, NON_SECURE));

		cm_el1_sysregs_context_save(NON_SECURE);

		/*
		 * Verify if there is a valid context to use, copy the
		 * operation type and parameters to the secure context
		 * and jump to the fast smc entry point in the secure
		 * payload. Entry into S-EL1 will take place upon exit
		 * from this function.
		 */
		assert(&tkcore_ctx->cpu_ctx ==
			cm_get_context_by_index(linear_id, SECURE));

		/* Set appropriate entry for SMC.
		 * We expect TKCORE to manage the PSTATE.I and PSTATE.F
		 * flags as appropriate.
		 */
		if (GET_SMC_TYPE(smc_fid) == SMC_TYPE_FAST) {
			cm_set_elr_el3(SECURE, (uint64_t)
					&tkcore_vectors->fast_smc_entry);
		} else {
			cm_set_elr_el3(SECURE, (uint64_t)
					&tkcore_vectors->std_smc_entry);
		}

		cm_el1_sysregs_context_restore(SECURE);
		cm_set_next_eret_context(SECURE);

		/* Propagate hypervisor client ID */
		write_ctx_reg(get_gpregs_ctx(&tkcore_ctx->cpu_ctx),
			CTX_GPREG_X7, read_ctx_reg(get_gpregs_ctx(handle),
			CTX_GPREG_X7));

		SMC_RET4(&tkcore_ctx->cpu_ctx, smc_fid, x1, x2, x3);
	}

	/*
	 * Returning from TKCORE
	 */

	switch (smc_fid) {
	/*
	 * TKCORE has finished initialising itself after a cold boot
	 */
	case TEESMC32_TKCORED_RETURN_ENTRY_DONE:
	case TEESMC64_TKCORED_RETURN_ENTRY_DONE:
		/*
		* Stash the TKCORE entry points information. This is done
		* only once on the primary cpu
		*/
		assert(tkcore_vectors == NULL);
		tkcore_vectors = (tkcore_vectors_t *) x1;

		/*
		 * TKCORE reports completion. The TKCORED must have initiated
		 * the original request through a synchronous entry into
		 * the SP. Jump back to the original C runtime context.
		 */
		tkcored_synchronous_sp_exit(tkcore_ctx, x1);


	/*
	 * TKCORE has finished turning itself on in response to an earlier
	 * psci cpu_on request
	 */
	case TEESMC32_TKCORED_RETURN_ON_DONE:
	case TEESMC64_TKCORED_RETURN_ON_DONE:

	/*
	 * TKCORE has finished turning itself off in response to an earlier
	 * psci cpu_off request.
	 */
	case TEESMC32_TKCORED_RETURN_OFF_DONE:
	case TEESMC64_TKCORED_RETURN_OFF_DONE:
	/*
	 * TKCORE has finished resuming itself after an earlier psci
	 * cpu_suspend request.
	 */
	case TEESMC32_TKCORED_RETURN_RESUME_DONE:
	case TEESMC64_TKCORED_RETURN_RESUME_DONE:
	/*
	 * TKCORE has finished suspending itself after an earlier psci
	 * cpu_suspend request.
	 */
	case TEESMC32_TKCORED_RETURN_SUSPEND_DONE:
	case TEESMC64_TKCORED_RETURN_SUSPEND_DONE:

		/*
		 * TKCORE reports completion. The TKCORED must have initiated the
		 * original request through a synchronous entry into TKCORE.
		 * Jump back to the original C runtime context, and pass x1 as
		 * return value to the caller
		 */
		tkcored_synchronous_sp_exit(tkcore_ctx, x1);

	/*
	 * TKCORE is returning from a call or being preemted from a call, in
	 * either case execution should resume in the normal world.
	 */
	case TEESMC32_TKCORED_RETURN_CALL_DONE:
	case TEESMC64_TKCORED_RETURN_CALL_DONE:
		/*
		 * This is the result from the secure client of an
		 * earlier request. The results are in x0-x3. Copy it
		 * into the non-secure context, save the secure state
		 * and return to the non-secure state.
		 */
		assert(handle == cm_get_context_by_index(linear_id, SECURE));

		cm_el1_sysregs_context_save(SECURE);

		/* Get a reference to the non-secure context */
		ns_cpu_context = cm_get_context_by_index(linear_id, NON_SECURE);
		assert(ns_cpu_context);

		/* Restore non-secure state */
		cm_el1_sysregs_context_restore(NON_SECURE);
		cm_set_next_eret_context(NON_SECURE);

		x1 &= ((1UL << 32) - 1);
		x2 &= ((1UL << 32) - 1);
		x3 &= ((1UL << 32) - 1);
		x4 &= ((1UL << 32) - 1);

		SMC_RET4(ns_cpu_context, x1, x2, x3, x4);

	/*
	 * TKCORE has finished handling a S-EL1 FIQ interrupt. Execution
	 * should resume in the normal world.
	 */
	case TEESMC32_TKCORED_RETURN_FIQ_DONE:
	case TEESMC64_TKCORED_RETURN_FIQ_DONE:
		/* Get a reference to the non-secure context */
		ns_cpu_context = cm_get_context_by_index(linear_id, NON_SECURE);
		assert(ns_cpu_context);

		/*
		 * Restore non-secure state. There is no need to save the
		 * secure system register context since TKCORE was supposed
		 * to preserve it during S-EL1 interrupt handling.
		 */
		cm_el1_sysregs_context_restore(NON_SECURE);
		cm_set_next_eret_context(NON_SECURE);

		SMC_RET0((uint64_t) ns_cpu_context);

	case TEESMC32_TKCORED_SECURE_SERVICE_REQUEST:
	case TEESMC64_TKCORED_SECURE_SERVICE_REQUEST:

		smc_fid = plat_tkcore_secure_service_request(x1, &x2, &x3);

		SMC_RET4(&tkcore_ctx->cpu_ctx, smc_fid, x1, x2, x3);

	default:
		assert(0);
	}
}

/* Define an TKCORED runtime service descriptor for fast SMC calls */
DECLARE_RT_SVC(
	tkcored_fast,

	OEN_TOS_START,
	OEN_TOS_END,
	SMC_TYPE_FAST,
	tkcored_setup,
	tkcored_smc_handler
);

/* Define an TKCORED runtime service descriptor for standard SMC calls */
DECLARE_RT_SVC(
	tkcored_std,

	OEN_TOS_START,
	OEN_TOS_END,
	SMC_TYPE_STD,
	NULL,
	tkcored_smc_handler
);
