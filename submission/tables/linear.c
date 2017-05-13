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

// helper struct to store statistics about collisions
typedef struct {
	// records total times at least 1 collision has occured during insersion
	int total_colls;
	// records what load the table was under when at least 1 collision occured
	// during insersion. index 0: load factor 0-10%, index 1: load_factor
	// 10-20%, ..., index 9: load_factor 90-100%
	int colls_by_load[10];
} Collisions;

// helper struct to store statistics about probe sequence
typedef struct {
	// records total probes made while inserting a key while table is under
	// a certian load factor. indexes correspond to same load factors in
	// 'colls_by_load'
	int nprobes_by_load[10];
	// records total number of keys that have inserted under load range, split
	// into 10% intervals same as 'colls_by_load'
	int nkeys_by_load[10];
} Probes;

// helper structure to store statistics gathered
typedef struct stats {
	Collisions coll;  // infomation about collisions during insersion
	Probes probes;    // infomation about the probes required to insert
} Stats;

// a hash table is an array of slots holding keys, along with a parallel array
// of boolean markers recording which slots are in use (true) or free (false)
// important because not-in-use slots might hold garbage data, as they may
// not have been initialised
struct linear_table {
	int64 *slots;	// array of slots holding keys
	bool  *inuse;	// is this slot in use or not?
	int size;		// the size of both of these arrays right now
	int load;		  // number of keys in the table right now
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
// function as we don't want these to reset when table is doubled
static void initialise_stats(LinearHashTable *table) {
	// set everything to 0
	table->stats.coll.total_colls = 0;
	int i;
	for (i = 0; i < 10; i++) {
		table->stats.coll.colls_by_load[i] = 0;
		table->stats.probes.nprobes_by_load[i] = 0;
		table->stats.probes.nkeys_by_load[i] = 0;
	}
}


// double the size of the internal table arrays and re-hash all
// keys in the old tables
static void double_table(LinearHashTable *table) {
	int64 *oldslots = table->slots;
	bool  *oldinuse = table->inuse;
	int oldsize = table->size;

	initialise_table(table, table->size * 2);

	int i;
	for (i = 0; i < oldsize; i++) {
		if (oldinuse[i] == true) {
			linear_hash_table_insert(table, oldslots[i]);
		}
	}

	free(oldslots);
	free(oldinuse);
}


// gets the index of arrays in our stats that corresponds to a load factor
static int get_stats_index(float load_factor) {
	if (load_factor <= 10.0) return 0;
	else if (load_factor <= 20.0) return 1;
	else if (load_factor <= 30.0) return 2;
	else if (load_factor <= 40.0) return 3;
	else if (load_factor <= 50.0) return 4;
	else if (load_factor <= 60.0) return 5;
	else if (load_factor <= 70.0) return 6;
	else if (load_factor <= 80.0) return 7;
	else if (load_factor <= 90.0) return 8;
	else return 9;
}


// update statistics about when collisions are likely to occur in the table
static void update_collision_stats(LinearHashTable *table) {
	assert(table);

	// calculate the load factor at the time of collision
	float load_factor = table->load * 100.0 / table->size;

	// get index that needs updating
	int index = get_stats_index(load_factor);

	// increment overall keys with at least 1 collision during insersion
	table->stats.coll.total_colls++;
	// mark what the load factor was when this collision occured
	table->stats.coll.colls_by_load[index]++;
}

static void update_probe_stats(LinearHashTable *table, int steps) {
	assert(table);

	// calculate the load factor at the time of collision
	float load_factor = table->load * 100.0 / table->size;

	// get index that needs updating
	int index = get_stats_index(load_factor);

	// update probes stats
	table->stats.probes.nkeys_by_load[index]++;
	table->stats.probes.nprobes_by_load[index] += steps;
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
	bool collision=false;

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

		// mark that there has been at least 1 collision when hashing this key
		collision = true;

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
		if (collision) update_collision_stats(table);
		update_probe_stats(table, steps);
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

	// print infomation about collisions

	int total_colls = table->stats.coll.total_colls;

	printf("Total collisions: %d\n", total_colls);
	int i, lower_bound, upper_bound, colls_this_load;
	float percent;
	for (i=0; i<10; i++) {
		// calculate stats for this load factor segment
		colls_this_load = table->stats.coll.colls_by_load[i];
		lower_bound = i * 10;
		upper_bound = (i + 1) * 10;
		// avoid 0 division
		if (total_colls > 0) {
			percent = colls_this_load * 100.0 / total_colls;
		} else percent = 0.0;

		printf("    Collsions when load factor %d%% - %d%%: %d (%.2f%%)\n",
							lower_bound, upper_bound, colls_this_load, percent);


	}

	printf("--- end stats ---\n");
}
