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
	int load;		// number of keys in the table right now
} InnerTable;

// a cuckoo hash table stores its keys in two inner tables
struct cuckoo_table {
	InnerTable *table1; // first table
	InnerTable *table2; // second table
	int size;			// size of the inner tables
};


// initialise a cuckoo hash table with 'size' slots in each table
CuckooHashTable *new_cuckoo_hash_table(int size) {
	// init reference point for the two tables
	CuckooHashTable *table = malloc(sizeof(*table));
	assert(table);

	// record size we're making tables
	table->size = size;

	// init the two inner tables
	table->table1 = new_inner_table(size);
	table->table2 = new_inner_table(size);

	return table;
}


// init a new inner_table of size n
InnerTable *new_inner_table(int size) {
	// init new InnerTable
	InnerTable *inner_table = malloc((sizeof *inner_table) * size);
	assert(inner_table);
	inner_table->load = 0;

	// init inuse and slots. calloc inuse to set everything to 0 initially.
	inner_table->slots = malloc((sizeof *inner_table->slots) * size);
	assert(inner_table->slots);
	inner_table->inuse = calloc(sizeof *inner_table->inuse, size);
	assert(inner_table->inuse);

	return inner_table;
}


// free all memory associated with 'table'
void free_cuckoo_hash_table(CuckooHashTable *table) {
	assert(table);

	free_inner_table(table->table1);
	free_inner_table(table->table2);
	free(table);
}


// free all memory associated with 'inner_table'
void free_inner_table(InnerTable *inner_table) {
	assert(inner_table);

	free(inner_table->slots);
	free(inner_table->inuse);
	free(inner_table);
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
		// if table too big need to double the size, then keep going
		if (steps >= max_steps) {
			// double table size and rehash everything
			double_table(table);
			max_steps = (table->size) / 2;

			// begin insersion into new table with table 1
			cur_table_num = 1;
		}

		// get vals from table 1 or 2 depending on which one we are inserting
		// into
		if (cur_table_num == 1) {
			cur_table = table->table1;
			// get hash of our key for table 1
			h = h1(key) % table->size;
		} else {
			cur_table = table->table2;
			// get hash of our key for table 2
			h = h2(key) % table->size;
		}

		// if destination slot is occupied need save it's val before moving on
		// so we can rehash it. else prepare loop to break and cur_table to
		// get the empty slot occupied
		if (cur_table->inuse[h]) {
			next_key = cur_table->slots[h];
		} else {
			cur_table->inuse[h] = true;
			cur_table->load += 1;
			next_key = false;
		}

		// insert key into it's desired slot, increment load
		cur_table->slots[h] = key;

		// set key to next key (if any)
		key = next_key;

		// switch next table to insert into
		cur_table_num = (cur_table_num == 1) ? 2: 1;

		// increment step counter
		steps += 1;
	}

	return true;
}

// double the size of inner tables within table, and reinsert everything
void double_table(CuckooHashTable *table) {
	assert(table);
	// save pointer of old inner tables and old size
	InnerTable *old_table1 = table->table1;
	InnerTable *old_table2 = table->table2;
	int old_size = table->size;

	// update table size
	table->size = (table->size) * 2;
	assert(table->size <= MAX_TABLE_SIZE);

	// replace inner tables
	table->table1 = new_inner_table(table->size);
	table->table2 = new_inner_table(table->size);

	int i;
	// reinsert everything into the table
	for (i = 0; i < old_size; i++) {
		if (old_table1->inuse[i]) {
			cuckoo_hash_table_insert(table, old_table1->slots[i]);
		}
		if (old_table2->inuse[i]) {
			cuckoo_hash_table_insert(table, old_table2->slots[i]);
		}
	}
	free_inner_table(old_table1);
	free_inner_table(old_table2);
}

// lookup whether 'key' is inside 'table'
// returns true if found, false if not
bool cuckoo_hash_table_lookup(CuckooHashTable *table, int64 key) {
	assert(table);
	int h;

	// calculate the address for this key in table 1
	h = h1(key) % table->size;

	// check if key in table 1
	if (table->table1->inuse[h] && table->table1->slots[h] == key) {
		return true;
	}

	// calculate the address for this key in table 2
	h = h2(key) % table->size;

	// check if key in table 2
	if (table->table2->inuse[h] && table->table2->slots[h] == key) {
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
	assert(table);
	printf("--- table stats ---\n");

	// print some information about the table
	printf("current size: %d slots\n", table->size);
	printf("table1 current load: %d items\n", table->table1->load);
	printf(" table1 load factor: %.3f%%\n", table->table1->load * 100.0 / table->size);
	printf("current table2 load: %d items\n", table->table2->load);
	printf(" table2 load factor: %.3f%%\n", table->table2->load * 100.0 / table->size);

	printf("--- end stats ---\n");
}
