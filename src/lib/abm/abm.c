#include <lib/abm/abm.h>

#include <core/init.h>
#include <datatypes/array.h>
#include <lib/topology/topology.h>
#include <lib/lib_internal.h>

#include <stdio.h>

#define agent_to_struct(agent)					\
	((struct abm_agent *)(((char *)(agent)) - offsetof(struct abm_agent, data)))

struct abm_leave_evt {
	abm_key_t key;
	lp_id_t dest;
};

/**
* Initializes the abm layer internals for all the lps hosted on the machine.
* This needs to be called before starting processing events, after basic
* initialization of the lps.
*/
void abm_lib_lp_init(void)
{
	struct lib_ctx *ctx = lib_ctx_get();
	ctx->region.next_key = lp_id_get();
	agent_map_init(&ctx->region.a_map);
}

void abm_lib_lp_fini(void)
{
	struct lib_ctx *ctx = lib_ctx_get();
	agent_map_fini(&ctx->region.a_map);
}


/**
* Handle an agent departure. This is called by the abm layer when a leave message is received.
* We expect the event to have the the agent key as payload.
*/
static void on_abm_leave(lp_id_t me, simtime_t now, const void *restrict content)
{
	const struct abm_leave_evt *restrict leave_evt = content;
	struct lib_ctx *ctx = lib_ctx_get();
	struct abm_agent *agent = agent_map_lookup(&ctx->region.a_map, leave_evt->key);
	if (!agent || agent->leave_time > now)
		// the exiting agent has been killed, already left or the agent is trying to leave too early
		return;

	ProcessEvent(me, now, ABM_LEAVING, &agent->data, abm_settings.agent_data_size, ctx->state_s);
	// the user could have killed the agent or postponed his movement
	agent = agent_map_lookup(&ctx->region.a_map, leave_evt->key);
	if (!agent || agent->leave_time > now)
		return;

	ScheduleNewEvent(leave_evt->dest, now, ABM_VISITING, &agent->key,
			sizeof(*agent) + abm_settings.agent_data_size - offsetof(struct abm_agent, key));

	agent_map_remove(&ctx->region.a_map, leave_evt->key);
	free(agent);
}

/**
* Handle an agent departure. This is called by the abm layer when a leave message is received.
* We expect the event to have the the agent key as payload.
*/
static void on_abm_visit(lp_id_t me, simtime_t now, const void *restrict content)
{
	struct abm_agent *ret = malloc(sizeof(*ret) + abm_settings.agent_data_size);
	if (ret == NULL) {
		perror("Failed allocation in the agent based layer");
		exit(-1);
	}
	memcpy(&ret->key, content, sizeof(*ret) + abm_settings.agent_data_size -
			offsetof(struct abm_agent, key));

	struct lib_ctx *ctx = lib_ctx_get();
	agent_map_add(&ctx->region.a_map, ret);

	ProcessEvent(me, now, ABM_VISITING, ret->data, abm_settings.agent_data_size, ctx->state_s);
}

bool IterAgents(agent_t **agent_p)
{
	static __thread map_size_t closure = 0;
	struct abm_region *region = &lib_ctx_get()->region;
	if (*agent_p == NULL)
		closure = 0;

	struct abm_agent *ret = agent_map_next(&region->a_map, &closure);

	*agent_p = ret;

	return agent_p != NULL;
}

unsigned CountAgents(void)
{
	return agent_map_count(&lib_ctx_get()->region.a_map);
}

agent_t *SpawnAgent(void)
{
	struct abm_region *region = &lib_ctx_get()->region;
	abm_key_t new_key = (region->next_key += n_lps);
	// new agent
	struct abm_agent *ret = malloc(sizeof(*ret) + abm_settings.agent_data_size);

	ret->key = new_key;
	ret->leave_time = -1;

	agent_map_add(&region->a_map, ret);

	return ret->data;
}

void KillAgent(agent_t *agent)
{
	struct abm_agent *agent_buf = agent_to_struct(agent);
	agent_map_remove(&lib_ctx_get()->region.a_map, agent_buf->key);
	free(agent_buf);
}

void ScheduleNewLeaveEvent(lp_id_t receiver, simtime_t timestamp, agent_t *agent)
{
	// we mark the agent with the intended leave time so we can later compare it
	// to check for spurious events
	struct abm_agent *agent_buf = agent_to_struct(agent);
	agent_buf->leave_time = timestamp;

	struct abm_leave_evt leave_evt = {agent_buf->key, receiver};
	ScheduleNewEvent(lp_id_get(), timestamp, ABM_LEAVING, &leave_evt, sizeof(leave_evt));
}

unsigned long long IdAgent(agent_t *agent)
{
	return agent_to_struct(agent)->key;
}
