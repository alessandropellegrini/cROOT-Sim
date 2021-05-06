/**
 * @file lib/abm/abm.h
 *
 * @brief Agent based library header
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <core/core.h>

#include <lib/abm/agent_map.h>

struct abm_region {
	struct agent_map a_map;
	abm_key_t next_key;
};

struct abm_agent {
	simtime_t leave_time;
	abm_key_t key;
	char data[];
};

enum {
	ABM_LEAVING,
	ABM_VISITING
};

void abm_lib_lp_init(void);
void abm_lib_lp_fini(void);

__attribute((weak)) extern struct abm_settings {
	const unsigned agent_data_size;
} abm_settings;

typedef void agent_t;

bool IterAgents(agent_t **agent);
unsigned CountAgents(void);
agent_t *SpawnAgent(void);
unsigned long long IdAgent(agent_t *agent);
void KillAgent(agent_t *agent);
void ScheduleNewLeaveEvent(lp_id_t receiver, simtime_t timestamp, agent_t *agent);
