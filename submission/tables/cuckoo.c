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

	// record size we're making tables
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
	assert(table);
	int h;
	int64 next_key;
	InnerTable* cur_table;

	// if key already in table return false
	if (cuckoo_hash_table_lookup(table, key) == true) {
		return false;
	}

	// count steps so we know when need to increase table size
	int steps = 0;
	int max_steps = (table->size) / 2;

	// keep track of which table we're inserting into
	int cur_table_num = 1;

	// while we have a key to hash keep going!
	while (key) {
		// if table too big need to double the size
		if (steps >= max_steps) {
			double_table(table);
			max_steps = (table->size) / 2;
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
			cur_table->inuse[h] = true;
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
	// finished inserting, return!
	return true;
}

// double the size of tables within table, rehashing everything
void double_table(CuckooHashTable *table) {
	// save copy of old data
	InnerTable *old_table1 = table->table1;
	InnerTable *old_table2 = table->table2;
	int old_size = table->size;

	// update table size
	table->size = (table->size) * 2;

	// create new tables and attach them to table
	InnerTable *table1 = malloc((sizeof *table1) * table->size);
	InnerTable *table2 = malloc((sizeof *table2) * table->size);
	table->table1 = table1;
	table->table2 = table2;


	int i;
	// insert everything from old tables into new table
	for (i = 0; i < old_size; i++) {
		if (old_table1->inuse[i] == true) {
			cuckoo_hash_table_insert(table, old_table1->slots[i]);
		}
		if (old_table2->inuse[i] == true) {
			cuckoo_hash_table_insert(table, old_table2->slots[i]);
		}
	}

	free(old_table1);
	free(old_table2);
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

	// calculate the address for this key in table 2
	h = h2(key) % table->size;

	// check if key in table 2
	if (table->table2->slots[h] == key) {
		return true;
	}

	// key is in neither of the tables
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
