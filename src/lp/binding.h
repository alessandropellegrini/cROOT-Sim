#pragma once

#include <core/core.h>
#include <stdint.h>

extern void binding_global_init(void);
extern void binding_init(void);
extern void binding_global_fini(void);

uint32_t *binding_map;

inline _Bool binding_lp_id_is_local(lp_id_t lp_id)
{
	return binding_map[lp_id] & 1u;
}

inline nid_t binding_lp_id_to_nid(lp_id_t lp_id)
{
	return binding_map[lp_id] >> 1;
}

inline rid_t binding_lp_id_to_rid(lp_id_t lp_id)
{
	return binding_map[lp_id] >> 1;
}
