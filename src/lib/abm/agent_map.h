/**
 * @file lib/abm/agent_map.h
 *
 * @brief Agent map header
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <stdint.h>
#include <datatypes/array.h>

#define MAX_LOAD_FACTOR 0.85
#define MIN_LOAD_FACTOR 0.05

typedef uint32_t map_size_t;
typedef map_size_t hash_t;
typedef uint64_t abm_key_t;

struct agent_map {
	map_size_t capacity_mo;
	map_size_t count;
	struct agent_map_node {
		hash_t hash;
		struct abm_agent *agent;
	} *nodes;
};

extern void agent_map_init(struct agent_map *a_map);
extern void agent_map_fini(struct agent_map *a_map);
extern map_size_t agent_map_count(const struct agent_map *a_map);
extern struct abm_agent *agent_map_next(const struct agent_map *a_map, map_size_t *closure);
extern void agent_map_add(struct agent_map *a_map, struct abm_agent *agent);
extern struct abm_agent *agent_map_lookup(const struct agent_map *a_map, abm_key_t key);
extern void agent_map_remove(struct agent_map *a_map, abm_key_t key);
