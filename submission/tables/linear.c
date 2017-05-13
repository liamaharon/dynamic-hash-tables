/* * * * * * * * *
 * Dynamic hash table using linear probing to resolve collisions
 *
 * created for COMP20007 Design of Algorithms - Assignment 2, 2017
 * by Matt Farrugia <matt.farrugia@unimelb.edu.au>
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>  // for memset

#include "linear.h"

// how many cells to advance at a time while looking for a free slot
#define STEP_SIZE 1
// how many slots we want to split our load factor into in our stats arrays
// higher number means more specific stats output
#define NUM_LOAD_FACTOR_SLOTS 50


// helper structure to store statistics gathered
typedef struct stats {
	// records what load the table was under when at least 1 collision occured
	// during insersion. index 0: load factor 0-5%, index 1: load_factor
	// 5-10%, ..., index 9: load_factor 95-100%
	int ncolls_by_load[NUM_LOAD_FACTOR_SLOTS];
	// records total probes made while inserting a key while table is under
	// a certian load factor. indexes correspond to same load factors in
	// 'ncolls_by_load'
	int nprobes_by_load[NUM_LOAD_FACTOR_SLOTS];
	// counts the number of times a key was inserted under particular load
	// factor. array split the same way as 'ncolls_by_load'
	int nkeys_by_load[NUM_LOAD_FACTOR_SLOTS];
} Stats;

// a hash table is an array of slots holding keys, along with a parallel array
// of boolean markers recording which slots are in use (true) or free (false)
// important because not-in-use slots might hold garbage data, as they may
// not have been initialised
struct linear_table {
	int64 *slots;	// array of slots holding keys
	bool  *inuse;	// is this slot in use or not?
	int size;		// the size of both of these arrays right now
	int load;		// number of keys in the table right now
	Stats stats;	// collection of statistics about this hash table
};


/* * * *
 * helper functions
 */

// set up the internals of a linear hash table struct with new
// arrays of size 'size'
static void initialise_table(LinearHashTable *table, int size) {
	assert(size < MAX_TABLE_SIZE && "error: table has grown too large!");

	table->slots = malloc((sizeof *table->slots) * size);
	assert(table->slots);
	table->inuse = malloc((sizeof *table->inuse) * size);
	assert(table->inuse);
	int i;
	for (i = 0; i < size; i++) {
		table->inuse[i] = false;
	}

	table->size = size;

	table->load = 0;
}

// set up the internals of the statistics of the table. putting in different
// function so it's not called when the table doubles
static void initialise_stats(LinearHashTable *table) {
	assert(table);

	// set everything to 0
	int i;
	for (i = 0; i < NUM_LOAD_FACTOR_SLOTS; i++) {
		table->stats.nkeys_by_load[i] = 0;
		table->stats.ncolls_by_load[i] = 0;
		table->stats.nprobes_by_load[i] = 0;
	}
}


// double the size of the internal table arrays and re-hash all
// keys in the old tables
static void double_table(LinearHashTable *table) {
	assert(table);

	int64 *oldslots = table->slots;
	bool  *oldinuse = table->inuse;
	int oldsize = table->size;

	initialise_table(table, table->size * 2);
	initialise_stats(table);

	int i;
	for (i = 0; i < oldsize; i++) {
		if (oldinuse[i] == true) {
			linear_hash_table_insert(table, oldslots[i]);
		}
	}

	free(oldslots);
	free(oldinuse);
}


// gets the index that corresponds to the slot in our stats array for the
// load factor input
static int get_stats_index(float load_factor) {
	int index;
	// set the step size
	int step_size = 100 / NUM_LOAD_FACTOR_SLOTS;
	int cur_chk = step_size;
	// keep increasing the max load factor that would return as we increase
	// index. when cur_check becomes > load factor we know we've incremented
	// index the right amount of times to insert into it.
	for (index=0; index<NUM_LOAD_FACTOR_SLOTS; index++) {
		if (load_factor <= cur_chk) return index;
		cur_chk += step_size;
	}
	// should never get to here
	assert(false && "make sure NUM_LOAD_FACTOR_SLOTS divides evently into 100");
	return -1;
}


// updates the statistics of the table
static void update_table_stats(LinearHashTable *table, int steps) {
	assert(table);
	bool collision=false;

	// check if there was a collision
	if (steps > 0) collision = true;

	// calculate the load factor at the time of collision
	float load_factor = table->load * 100.0 / table->size;

	// get index of load factor that needs updating
	int index = get_stats_index(load_factor);

	// if there's been a collision mark where it was
	if (collision) table->stats.ncolls_by_load[index]++;

	// increment how many probes it took to insert the key
	table->stats.nprobes_by_load[index] += steps;

	// mark that a key has been inserted under the particular load factor
	table->stats.nkeys_by_load[index]++;
}


// print infomation about collisions
static void print_collisions_stats(LinearHashTable *table) {
	assert(table);

	int i, lower_bound, upper_bound, colls_this_load, nkeys_this_load;
	float percent;

	// calculate the percent size of each slot in the stats arrays
	int percent_per_slot = 100 / NUM_LOAD_FACTOR_SLOTS;

	printf("\nCollisions during insert when load factor is\n");
	for (i=0; i<NUM_LOAD_FACTOR_SLOTS; i++) {
		// calculate stats for this load factor
		nkeys_this_load = table->stats.nkeys_by_load[i];
		colls_this_load = table->stats.ncolls_by_load[i];
		// get the bounds of the current index
		lower_bound = i * percent_per_slot;
		upper_bound = (i + 1) * percent_per_slot;
		// avoid 0 division
		if (nkeys_this_load > 0) {
			percent = colls_this_load * 100.0 / nkeys_this_load;
		} else percent = 0.0;

		// print collision count and chance
		printf("    %d%% - %d%%: %d (%.2f%% chance)\n",
			lower_bound, upper_bound, colls_this_load, percent);
	}
}

// print infomation about average probe sequence length
static void print_probe_stats(LinearHashTable *table) {
	assert(table);

	int i, lower_bound, upper_bound, probes_this_load, nkeys_this_load;
	float avg_probe;

	int percent_per_slot = 100 / NUM_LOAD_FACTOR_SLOTS;

	printf("\nAverage probe sequence length when load factor is\n");
	for (i=0; i<NUM_LOAD_FACTOR_SLOTS; i++) {
		// calculate stats for this load factor
		// get the bounds of the current index
		lower_bound = i * percent_per_slot;
		upper_bound = (i + 1) * percent_per_slot;
		probes_this_load = table->stats.nprobes_by_load[i];
		nkeys_this_load = table->stats.nkeys_by_load[i];
		// avoid 0 division
		if (nkeys_this_load > 0) {
			avg_probe = probes_this_load * 1.0 / nkeys_this_load;
		} else {
			avg_probe = 0.0;
		}
		// finally print the avg probe sequence length for this load factor
		printf("    %d%% - %d%%: %.2f\n",
			lower_bound, upper_bound, avg_probe);
	}

	printf("--- end stats ---\n");
}


/* * * *
 * all functions
 */

// initialise a linear probing hash table with initial size 'size'
LinearHashTable *new_linear_hash_table(int size) {
	LinearHashTable *table = malloc(sizeof *table);
	assert(table);

	// set up the internals of the table struct with arrays of size 'size'
	initialise_table(table, size);
	// set up the stats of the table
	initialise_stats(table);

	return table;
}


// free all memory associated with 'table'
void free_linear_hash_table(LinearHashTable *table) {
	assert(table != NULL);

	// free the table's arrays
	free(table->slots);
	free(table->inuse);

	// free the table struct itself
	free(table);
}


// insert 'key' into 'table', if it's not in there already
// returns true if insertion succeeds, false if it was already in there
bool linear_hash_table_insert(LinearHashTable *table, int64 key) {
	assert(table != NULL);

	// need to count our steps to make sure we recognise when the table is full
	int steps = 0;

	// calculate the initial address for this key
	int h = h1(key) % table->size;

	// step along the array until we find a free space (inuse[]==false),
	// or until we visit every cell
	while (table->inuse[h] && steps < table->size) {
		if (table->slots[h] == key) {
			// this key already exists in the table! no need to insert
			return false;
		}

		// else, keep stepping through the table looking for a free slot
		h = (h + STEP_SIZE) % table->size;
		steps++;
	}

	// if we used up all of our steps, then we're back where we started and the
	// table is full
	if (steps == table->size) {
		// let's make some more space and then try to insert this key again!
		double_table(table);
		return linear_hash_table_insert(table, key);

	} else {
		// otherwise, we have found a free slot! insert this key right here
		table->slots[h] = key;
		table->inuse[h] = true;
		table->load++;
		// update table stats before returning
		update_table_stats(table, steps);
		return true;
	}
}


// lookup whether 'key' is inside 'table'
// returns true if found, false if not
bool linear_hash_table_lookup(LinearHashTable *table, int64 key) {
	assert(table != NULL);

	// need to count our steps to make sure we recognise when the table is full
	int steps = 0;

	// calculate the initial address for this key
	int h = h1(key) % table->size;

	// step along until we find a free space (inuse[]==false), or until we
	// visit every cell
	while (table->inuse[h] && steps < table->size) {

		if (table->slots[h] == key) {
			// found the key!
			return true;
		}

		// keep stepping
		h = (h + STEP_SIZE) % table->size;
		steps++;
	}

	// we have either searched the whole table or come back to where we started
	// either way, the key is not in the hash table
	return false;
}


// print the contents of 'table' to stdout
void linear_hash_table_print(LinearHashTable *table) {
	assert(table != NULL);

	printf("--- table size: %d\n", table->size);

	// print header
	printf("   address | key\n");

	// print the rows of the hash table
	int i;
	for (i = 0; i < table->size; i++) {

		// print the address
		printf(" %9d | ", i);

		// print the contents of the slot
		if (table->inuse[i]) {
			printf("%llu\n", table->slots[i]);
		} else {
			printf("-\n");
		}
	}

	printf("--- end table ---\n");
}


// print some statistics about 'table' to stdout
void linear_hash_table_stats(LinearHashTable *table) {
	assert(table != NULL);
	printf("--- table stats ---\n");

	// print some information about the table
	printf("current size: %d slots\n", table->size);
	printf("current load: %d items\n", table->load);
	printf(" load factor: %.3f%%\n", table->load * 100.0 / table->size);
	printf("   step size: %d slots\n", STEP_SIZE);

	// print some infomation about collisions
	print_collisions_stats(table);

	// print some infomation about average probe sequence length
	print_probe_stats(table);
}
