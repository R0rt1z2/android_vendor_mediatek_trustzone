#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <context_mgmt.h>
#include <debug.h>
#include <platform.h>
#include <tkcore.h>
#include "tkcored_private.h"

static void tkcored_cpu_on_handler(uint64_t target_cpu)
{

	//uint64_t mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(target_cpu);

	tkcore_context_t *tkcore_ctx = &tkcored_sp_context[linear_id];

	set_tkcore_pstate(tkcore_ctx->state, TKCORE_PSTATE_ON_PENDING);
}

static int32_t tkcored_cpu_off_handler(uint64_t cookie)
{
	int32_t rc = 0;
	uint64_t mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(mpidr);

	tkcore_context_t *tkcore_ctx = &tkcored_sp_context[linear_id];

	assert(tkcore_vectors);
	assert(get_tkcore_pstate(tkcore_ctx->state) == TKCORE_PSTATE_ON);

	/* Program the entry point and enter TKCORE */
	cm_set_elr_el3(SECURE, (uint64_t) &tkcore_vectors->cpu_off_entry);
	rc = tkcored_synchronous_sp_entry(tkcore_ctx);

	/*
	 * Read the response from TKCORE. A non-zero return means that
	 * something went wrong while communicating with TKCORE.
	 */
	if (rc != 0)
		panic();

	/*
	 * Reset TKCORE's context for a fresh start when this cpu is turned on
	 * subsequently.
	 */
	set_tkcore_pstate(tkcore_ctx->state, TKCORE_PSTATE_OFF);

	 return 0;
}

static void tkcored_cpu_suspend_handler(uint64_t power_state)
{
	int32_t rc = 0;
	uint64_t mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(mpidr);
	tkcore_context_t *tkcore_ctx = &tkcored_sp_context[linear_id];

	assert(tkcore_vectors);
	assert(get_tkcore_pstate(tkcore_ctx->state) == TKCORE_PSTATE_ON);

	/* Program the entry point, power_state parameter and enter TKCORE */
	write_ctx_reg(get_gpregs_ctx(&tkcore_ctx->cpu_ctx),
		      CTX_GPREG_X0,
		      power_state);
	cm_set_elr_el3(SECURE, (uint64_t) &tkcore_vectors->cpu_suspend_entry);
	rc = tkcored_synchronous_sp_entry(tkcore_ctx);

	/*
	 * Read the response from TKCORE. A non-zero return means that
	 * something went wrong while communicating with TKCORE.
	 */
	if (rc != 0)
		panic();

	/* Update its context to reflect the state TKCORE is in */
	set_tkcore_pstate(tkcore_ctx->state, TKCORE_PSTATE_SUSPEND);
}

static void tkcored_cpu_on_finish_handler(uint64_t cookie)
{
	int32_t rc = 0;
	uint64_t mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(mpidr);
	tkcore_context_t *tkcore_ctx = &tkcored_sp_context[linear_id];

	assert(tkcore_vectors);
	assert(get_tkcore_pstate(tkcore_ctx->state) == TKCORE_PSTATE_ON_PENDING);

	//tkcored_init_core(mpidr);

	/* Initialise this cpu's secure context */
	tkcored_init_secure_context((uint64_t)&tkcore_vectors->cpu_on_entry,
				   tkcored_rw, mpidr, tkcore_ctx);

	/* Enter TKCORE */
	rc = tkcored_synchronous_sp_entry(tkcore_ctx);

	/*
	 * Read the response from TKCORE. A non-zero return means that
	 * something went wrong while communicating with TKCORE.
	 */
	if (rc != 0)
		panic();

	/* Update its context to reflect the state TKCORE is in */
	set_tkcore_pstate(tkcore_ctx->state, TKCORE_PSTATE_ON);
}

static void tkcored_cpu_suspend_finish_handler(uint64_t suspend_level)
{
	int32_t rc = 0;
	uint64_t mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(mpidr);

	tkcore_context_t *tkcore_ctx = &tkcored_sp_context[linear_id];

	assert(tkcore_vectors);
	assert(get_tkcore_pstate(tkcore_ctx->state) == TKCORE_PSTATE_SUSPEND);

	/* Program the entry point, suspend_level and enter the SP */
	write_ctx_reg(get_gpregs_ctx(&tkcore_ctx->cpu_ctx),
		      CTX_GPREG_X0,
		      suspend_level);
	cm_set_elr_el3(SECURE, (uint64_t) &tkcore_vectors->cpu_resume_entry);
	rc = tkcored_synchronous_sp_entry(tkcore_ctx);

	/*
	 * Read the response from TKCORE. A non-zero return means that
	 * something went wrong while communicating with TKCORE.
	 */
	if (rc != 0)
		panic();

	/* Update its context to reflect the state TKCORE is in */
	set_tkcore_pstate(tkcore_ctx->state, TKCORE_PSTATE_ON);
}

static int32_t tkcored_cpu_migrate_info(uint64_t *resident_cpu)
{
	return TKCORE_MIGRATE_INFO;
}

const spd_pm_ops_t tkcored_pm = {
	tkcored_cpu_on_handler,
	tkcored_cpu_off_handler,
	tkcored_cpu_suspend_handler,
	tkcored_cpu_on_finish_handler,
	tkcored_cpu_suspend_finish_handler,
	NULL,
	tkcored_cpu_migrate_info
};

