#include <lp/binding.h>

#include <lp/lp.h>

uint32_t *binding_map;

void binding_global_init(void)
{
	binding_map = mm_alloc(sizeof(*binding_map) * n_lps);
	memset(binding_map, 0, sizeof(*binding_map) * n_lps);
}

void binding_init(void)
{
	for (uint64_t i = lid_thread_first; i < lid_thread_end; ++i)
		binding_map[i] = (rid << 1U) + 1U;

	// TODO init remote nodes initial bindings
}

void binding_global_fini(void)
{
	mm_free(binding_map);
}
