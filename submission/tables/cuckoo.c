/* * * * * * * * *
* Dynamic hash table using cuckoo hashing, resolving collisions by switching
* keys between two tables with two separate hash functions
*
* created for COMP20007 Design of Algorithms - Assignment 2, 2017
* by Liam Aharon
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "cuckoo.h"

// an inner table represents one of the two internal tables for a cuckoo
// hash table. it stores two parallel arrays: 'slots' for storing keys and
// 'inuse' for marking which entries are occupied
typedef struct inner_table {
	int64 *slots;	// array of slots holding keys
	bool  *inuse;	// is this slot in use or not?
} InnerTable;

// a cuckoo hash table stores its keys in two inner tables
struct cuckoo_table {
	InnerTable *table1; // first table
	InnerTable *table2; // second table
	int size;			// size of each table
};


// initialise a cuckoo hash table with 'size' slots in each table
CuckooHashTable *new_cuckoo_hash_table(int size) {
	// init reference point for the two tables
	CuckooHashTable *table = malloc(sizeof(*table));
	assert(table);

	// record size we're making the tables
	table->size = size;

	// init the two tables
	table->table1 = malloc((sizeof *table->table1) * size);
	assert(table->table1);
	table->table2 = malloc((sizeof *table->table2) * size);
	assert(table->table2);

	// init contents of each table
	table->table1->inuse = malloc((sizeof *table->table1->inuse) * size);
	assert(table->table1->inuse);
	table->table1->slots = malloc((sizeof *table->table1->slots) * size);
	assert(table->table1->slots);
	table->table2->inuse = malloc((sizeof *table->table2->inuse) * size);
	assert(table->table2->inuse);
	table->table2->slots = malloc((sizeof *table->table2->slots) * size);
	assert(table->table2->slots);

	return table;
}


// free all memory associated with 'table'
void free_cuckoo_hash_table(CuckooHashTable *table) {
	fprintf(stderr, "not yet implemented\n");
}


// insert 'key' into 'table', if it's not in there already
// returns true if insertion succeeds, false if it was already in there
bool cuckoo_hash_table_insert(CuckooHashTable *table, int64 key) {
	int h;
	int64 next_key;
	InnerTable* cur_table;
	assert(table);

	// if key already in table return false
	if (cuckoo_hash_table_lookup(table, key) == true) {
		return false;
	}

	// count steps so we know if table is getting full and need to increase size
	int steps = 0;
	int max_steps = (table->size) / 2;

	// keep track of which table we're inserting into
	int cur_table_num = 1;

	// while we have a key to hash keep going!
	while (key) {

		if (steps >= max_steps) {
			printf("steps >= max_steps\n");
		}

		// setup depending on if we're inserting into table 1 or 2
		if (cur_table_num == 1) {
			// use table 1
			cur_table = table->table1;
			// get hash of our key for table 1
			h = h1(key) % table->size;

		} else {
			// use table 2
			cur_table = table->table2;
			// get hash of our key for table 2
			h = h2(key) % table->size;
		}

		// if destination slot is occupied need save it's val before moving on
		// so we can rehash it. else set to false so loop breaks
		if (cur_table->inuse[h]) {
			next_key = cur_table->slots[h];
		} else {
			next_key = false;
		}

		// insert key into it's desired slot
		cur_table->slots[h] = key;

		// set key to next key (if any)
		key = next_key;

		// switch next table to insert into
		cur_table_num = (cur_table_num == 1) ? 2: 1;

		// increment step counter
		steps += 1;
	}

	return false;
}


// lookup whether 'key' is inside 'table'
// returns true if found, false if not
bool cuckoo_hash_table_lookup(CuckooHashTable *table, int64 key) {
	assert(table);
	int h;

	// calculate the address for this key in table 1
	h = h1(key) % table->size;

	// check if key in table 1
	if (table->table1->slots[h] == key) {
		return true;
	}

	// key not in table 1, check if it's in table 2

	// calculate the address for this key in table 2
	h = h2(key) % table->size;

	// check if key in table 2
	if (table->table2->slots[h] == key) {
		return true;
	}

	// key in in neither of the tables
	return false;
}


// print the contents of 'table' to stdout
void cuckoo_hash_table_print(CuckooHashTable *table) {
	assert(table);
	printf("--- table size: %d\n", table->size);

	// print header
	printf("                    table one         table two\n");
	printf("                  key | address     address | key\n");

	// print rows of each table
	int i;
	for (i = 0; i < table->size; i++) {

		// table 1 key
		if (table->table1->inuse[i]) {
			printf(" %20llu ", table->table1->slots[i]);
		} else {
			printf(" %20s ", "-");
		}

		// addresses
		printf("| %-9d %9d |", i, i);

		// table 2 key
		if (table->table2->inuse[i]) {
			printf(" %llu\n", table->table2->slots[i]);
		} else {
			printf(" %s\n",  "-");
		}
	}

	// done!
	printf("--- end table ---\n");
}


// print some statistics about 'table' to stdout
void cuckoo_hash_table_stats(CuckooHashTable *table) {
	fprintf(stderr, "not yet implemented\n");
}
