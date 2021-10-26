#include <lp/thief.h>

#include <core/core.h>
#include <lp/binding.h>

#include <stdatomic.h>
#include <stdlib.h>


void thief_on_gvt(void)
{
	static atomic_uint thief_count;

	unsigned i = atomic_fetch_add_explicit(&thief_count, 1u, memory_order_relaxed);

	// the fastest thread steals one random LP
	if (i == 0) {
		binding_map[random() % n_lps] = (rid << 1U) + 1U;
	} else if (i == n_threads - 1) {
		atomic_store_explicit(&thief_count, 0, memory_order_relaxed);
	}
}
