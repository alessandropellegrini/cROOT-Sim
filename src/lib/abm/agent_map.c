#include <lib/abm/agent_map.h>

#include <lib/abm/abm.h>

#include <memory.h>
#include <stdint.h>
#include <stdio.h>

#define MAP_INVALID_I UINT32_MAX

// must be a power of two
#define MAP_INITIAL_CAPACITY 8
#define DIB(curr_i, hash, capacity_mo) 	(((curr_i) - (hash)) & (capacity_mo))
#define SWAP_VALUES(a, b) 						\
	do {								\
		__typeof(a) _tmp = (a);					\
		(a) = (b);						\
		(b) = _tmp;						\
	} while(0)

// Adapted from http://xorshift.di.unimi.it/splitmix64.c PRNG,
// written by Sebastiano Vigna (vigna@acm.org)
// TODO benchmark further and select a possibly better hash function
static inline hash_t get_hash(abm_key_t key)
{
	uint64_t z = key + 0x9e3779b97f4a7c15;
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return (hash_t)((z ^ (z >> 31)) >> 32);
}

void agent_map_init(struct agent_map *a_map)
{
	// this is the effective capacity_minus_one
	// this trick saves us some subtractions when we
	// use the capacity as a bitmask to
	// select the relevant hash bits for table indexing
	a_map->capacity_mo = MAP_INITIAL_CAPACITY - 1;
	a_map->count = 0;
	a_map->nodes = malloc(sizeof(*a_map->nodes) * MAP_INITIAL_CAPACITY);
	if (a_map->nodes == NULL) {
		perror("Failed allocation in the agent hash map");
		exit(-1);
	}
	memset(a_map->nodes, 0, sizeof(*a_map->nodes) * MAP_INITIAL_CAPACITY);
}

void agent_map_fini(struct agent_map *a_map)
{
	free(a_map->nodes);
}

static void agent_map_insert_hashed(struct agent_map *a_map, hash_t hash,
		struct abm_agent *agent)
{
	struct agent_map_node *nodes = a_map->nodes;
	map_size_t capacity_mo = a_map->capacity_mo;
	// since capacity_mo is 2^n - 1 for some n
	// this effectively is a modulo 2^n
	map_size_t i = hash & capacity_mo;
	// dib stays for distance from initial bucket
	map_size_t dib = 0;

	// linear probing with robin hood hashing
	// https://cs.uwaterloo.ca/research/tr/1986/CS-86-14.pdf by Pedro Celis
	while (1) {
		if (nodes[i].agent == NULL) {
			// found an empty cell, put the pair here and we're done
			nodes[i].hash = hash;
			nodes[i].agent = agent;
			return;
		} else if (dib > DIB(i, nodes[i].hash, capacity_mo)) {
			// found a "richer" cell, swap the pairs and continue looking for a hole
			dib = DIB(i, nodes[i].hash, capacity_mo);
			SWAP_VALUES(nodes[i].hash, hash);
			SWAP_VALUES(nodes[i].agent, agent);
		}
		i = (i + 1) & capacity_mo;
		++dib;
	}
}

static void agent_map_realloc_rehash(struct agent_map *a_map)
{
	// helper pointers to iterate over the old array
	struct agent_map_node *rmv = a_map->nodes;
	// instantiates new array
	a_map->nodes = malloc(sizeof(*a_map->nodes) * (a_map->capacity_mo + 1));
	if (a_map->nodes == NULL) {
		perror("Failed allocation in the agent hash map");
		exit(-1);
	}
	memset(a_map->nodes, 0, sizeof(*a_map->nodes) * (a_map->capacity_mo + 1));
	// rehash the old array elements
	for (map_size_t i = 0, j = 0; j < a_map->count; ++i, ++j) {
		while (rmv[i].agent == NULL)
			++i;

		agent_map_insert_hashed(a_map, rmv[i].hash, rmv[i].agent);
	}
	// free the old array
	free(rmv);
}

static void agent_map_expand(struct agent_map *a_map)
{
	if(MAX_LOAD_FACTOR * a_map->capacity_mo >= a_map->count)
		return;

	a_map->capacity_mo = 2 * a_map->capacity_mo + 1;
	agent_map_realloc_rehash(a_map);
}

static void agent_map_shrink(struct agent_map *a_map)
{
	if(MIN_LOAD_FACTOR * a_map->capacity_mo <= a_map->count ||
			a_map->capacity_mo <= MAP_INITIAL_CAPACITY)
		return;

	a_map->capacity_mo /= 2;
	agent_map_realloc_rehash(a_map);
}

void agent_map_add(struct agent_map *a_map, struct abm_agent *agent)
{
	agent_map_expand(a_map);
	agent_map_insert_hashed(a_map, get_hash(agent->key), agent);
	a_map->count++;
}

static map_size_t agent_map_index_lookup(const struct agent_map *a_map, abm_key_t key)
{
	struct agent_map_node *nodes = a_map->nodes;
	map_size_t capacity_mo = a_map->capacity_mo;

	hash_t cur_hash = get_hash(key);
	map_size_t i = cur_hash & capacity_mo;
	map_size_t dib = 0;

	do {
		if (nodes[i].agent == NULL)
			// we found a hole where we expected something, the pair hasn't been found
			return MAP_INVALID_I;
		//  the more expensive comparison with the key is done only if necessary
		else if(nodes[i].hash == cur_hash && nodes[i].agent->key == key)
			// we found a pair with coinciding keys, return the index
			return i;

		i = (i + 1) & capacity_mo;
		++dib;
	} while (dib <= DIB(i, nodes[i].hash, capacity_mo));
	// we found a node- with lower DIB than expected: the wanted key isn't here
	// (else it would have finished here during its insertion).
	return MAP_INVALID_I;
}

struct abm_agent *agent_map_lookup(const struct agent_map *a_map, abm_key_t key)
{
	// find the index of the wanted key
	map_size_t i = agent_map_index_lookup(a_map, key);
	// return the pair if successful
	return i == MAP_INVALID_I ? NULL : a_map->nodes[i].agent;
}

void agent_map_remove(struct agent_map *a_map, abm_key_t key)
{
	// find the index of the wanted key
	map_size_t i = agent_map_index_lookup(a_map, key);
	// if unsuccessful we're done, nothing to remove here!
	if (i == MAP_INVALID_I) return;

	struct agent_map_node *nodes = a_map->nodes;
	map_size_t capacity_mo = a_map->capacity_mo;
	map_size_t j = i;
	// backward shift to restore the table state as if the insertion never happened:
	// http://codecapsule.com/2013/11/17/robin-hood-hashing-backward-shift-deletion by Emmanuel Goossaert
	do{ // the first iteration is necessary since the removed element is always overwritten somehow
		j = (j + 1) & capacity_mo;
	} while (nodes[j].agent != NULL && DIB(j, nodes[j].hash, capacity_mo));
	// we finally found out the end of the displaced sequence of nodes,
	// since we either found an empty slot or we found a node residing in his correct position

	--j; // we now point to the last element to move
	j &= capacity_mo;

	if (j >= i) {
		// we simply move the nodes one slot back
		memmove(&nodes[i], &nodes[i + 1], (j - i) * sizeof(*a_map->nodes));
	} else {
		// we wrapped around the table: move the first part back
		memmove(&nodes[i], &nodes[i + 1], (capacity_mo - i) * sizeof(*a_map->nodes));
		// move the first node to the last slot in the table
		memcpy(&nodes[capacity_mo], &nodes[0], sizeof(*a_map->nodes));
		// move the remaining stuff at the table beginning
		memmove(&nodes[0], &nodes[1], j * sizeof(*a_map->nodes));
	}

	// we clear the last moved slot (if we didn't move anything this clears the removed entry)
	nodes[j].agent = NULL;
	a_map->count--;

	// shrink the table if necessary
	agent_map_shrink(a_map);
}

map_size_t agent_map_count(const struct agent_map *a_map)
{
	return a_map->count;
}

struct abm_agent *agent_map_next(const struct agent_map *a_map, map_size_t *closure)
{
	for (map_size_t i = *closure; i < a_map->count; ++i)
		if (a_map->nodes[i].agent != NULL)
			return a_map->nodes[i++].agent;
	return NULL;
}
