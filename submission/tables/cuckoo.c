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
	fprintf(stderr, "not yet implemented\n");
	return false;
}


// lookup whether 'key' is inside 'table'
// returns true if found, false if not
bool cuckoo_hash_table_lookup(CuckooHashTable *table, int64 key) {
	fprintf(stderr, "not yet implemented\n");
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
