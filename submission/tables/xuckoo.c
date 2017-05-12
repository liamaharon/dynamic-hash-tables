/* * * * * * * * *
* Dynamic hash table using a combination of extendible hashing and cuckoo
* hashing with a single keys per bucket, resolving collisions by switching keys
* between two tables with two separate hash functions and growing the tables
* incrementally in response to cycles
*
* created for COMP20007 Design of Algorithms - Assignment 2, 2017
* by ...
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "xuckoo.h"

// macro to calculate the rightmost n bits of a number x
#define rightmostnbits(n, x) (x) & ((1 << (n)) - 1)

// a bucket stores a single key (full=true) or is empty (full=false)
// it also knows how many bits are shared between possible keys, and the first
// table address that references it
typedef struct bucket {
	int id;		// a unique id for this bucket, equal to the first address
				// in the table which points to it
	int depth;	// how many hash value bits are being used by this bucket
	bool full;	// does this bucket contain a key
	int64 key;	// the key stored in this bucket
} Bucket;

// an inner table is an extendible hash table with an array of slots pointing
// to buckets holding up to 1 key, along with some information about the number
// of hash value bits to use for addressing
typedef struct inner_table {
	Bucket **buckets;	// array of pointers to buckets
	int size;			// how many entries in the table of pointers (2^depth)
	int depth;			// how many bits of the hash value to use (log2(size))
	int nkeys;			// how many keys are being stored in the table
} InnerTable;

// a xuckoo hash table is just two inner tables for storing inserted keys
struct xuckoo_table {
	InnerTable *table1;
	InnerTable *table2;
};

/* * * *
 * helper functions
 */

 // create a new bucket first referenced from 'first_address', based on 'depth'
 // bits of its keys' hash values
Bucket *new_bucket(int first_address, int depth) {
	Bucket *bucket = malloc(sizeof *bucket);
	assert(bucket);

	bucket->id = first_address;
	bucket->depth = depth;
	bucket->full = false;

	return bucket;
}

// double the table of bucket pointers, duplicating the bucket pointers in the
// first half into the new second half of the table
static void double_table(InnerTable *inner_table) {
	int size = inner_table->size * 2;
	assert(size < MAX_TABLE_SIZE && "error: inner_table has grown too large!");

	// get a new array of twice as many bucket pointers, and copy pointers down
	inner_table->buckets = realloc(inner_table->buckets, (sizeof *inner_table->buckets) * size);
	assert(inner_table->buckets);
	int i;
	for (i = 0; i < inner_table->size; i++) {
		inner_table->buckets[inner_table->size + i] = inner_table->buckets[i];
	}

	// finally, increase the table size and the depth we are using to hash keys
	inner_table->size = size;
	inner_table->depth++;
}

// reinsert a key into the hash table after splitting a bucket --- we can assume
// that there will definitely be space for this key because it was already
// inside the hash table previously
// use 'xtndbl1_hash_table_insert()' instead for inserting new keys
static void reinsert_key(InnerTable *inner_table, int64 key) {
	int address = rightmostnbits(inner_table->depth, h1(key));
	inner_table->buckets[address]->key = key;
	inner_table->buckets[address]->full = true;
}

// split the bucket in 'table' at address 'address', growing table if necessary
static void split_bucket(InnerTable *inner_table, int address) {

	// FIRST,
	// do we need to grow the table?
	if (inner_table->buckets[address]->depth == inner_table->depth) {
		// yep, this bucket is down to its last pointer
		double_table(inner_table);
	}
	// either way, now it's time to split this bucket


	// SECOND,
	// create a new bucket and update both buckets' depth
	Bucket *bucket = inner_table->buckets[address];
	int depth = bucket->depth;
	int first_address = bucket->id;

	int new_depth = depth + 1;
	bucket->depth = new_depth;

	// new bucket's first address will be a 1 bit plus the old first address
	int new_first_address = 1 << depth | first_address;
	Bucket *newbucket = new_bucket(new_first_address, new_depth);

	// THIRD,
	// redirect every second address pointing to this bucket to the new bucket
	// construct addresses by joining a bit 'prefix' and a bit 'suffix'
	// (defined below)

	// suffix: a 1 bit followed by the previous bucket bit address
	int bit_address = rightmostnbits(depth, first_address);
	int suffix = (1 << depth) | bit_address;

	// prefix: all bitstrings of length equal to the difference between the new
	// bucket depth and the table depth
	// use a for loop to enumerate all possible prefixes less than maxprefix:
	int maxprefix = 1 << (inner_table->depth - new_depth);

	int prefix;
	for (prefix = 0; prefix < maxprefix; prefix++) {

		// construct address by joining this prefix and the suffix
		int a = (prefix << new_depth) | suffix;

		// redirect this table entry to point at the new bucket
		inner_table->buckets[a] = newbucket;
	}

	// FINALLY,
	// filter the key from the old bucket into its rightful place in the new
	// table (which may be the old bucket, or may be the new bucket)

	// remove and reinsert the key
	int64 key = bucket->key;
	bucket->full = false;
	reinsert_key(inner_table, key);
}

// init a new inner_table
InnerTable *new_inner_table() {
	// init new InnerTable
	InnerTable *inner_table = malloc((sizeof *inner_table) * size);
	assert(inner_table);
	inner_table->depth = 0;
	inner_table->size = 1;

	inner_table->buckets = malloc(sizeof *inner_table->buckets);
	assert(table->buckets);
	inner_table->buckets[0] = new_bucket(0, 0);

	return inner_table;
 }


// initialise an extendible cuckoo hash table
XuckooHashTable *new_xuckoo_hash_table() {
	// create new table
	XuckooHashTable *table = malloc(sizeof *table);
	assert(table);

	// create new inner tables
	table->table1 = new_inner_table();
	table->table2 = new_inner_table();

	return table;
}



// free all memory associated with 'table'
void free_xuckoo_hash_table(XuckooHashTable *table) {
	assert(table);

	free_inner_table(table->table1);
	free_inner_table(table->table2);

	free(table);
}

// free all memory associated with the inner table
void free_inner_table(InnerTable *inner_table) {
	assert(inner_table);

	// loop backwards through the array of pointers, freeing buckets only as we
	// reach their first reference
	// (if we loop through forwards, we wouldn't know which reference was last)
	int i;
	for (i = inner_table->size-1; i >= 0; i--) {
		if (inner_table->buckets[i]->id == i) {
			free(inner_table->buckets[i]);
		}
	}

	// free the array of bucket pointers
	free(inner_table->buckets);

	// free the table struct itself
	free(inner_table);
}


// insert 'key' into 'table', if it's not in there already
// returns true if insertion succeeds, false if it was already in there
bool xuckoo_hash_table_insert(XuckooHashTable *table, int64 key) {
	assert(table);
	int hash, address;
	int64 next_key;

	// is key already in table?
	if (xuckoo_hash_table_lookup(table, key) == true) {
		return false;
	}

	// count steps so we know when need to increase table size
	int steps = 0;
	int max_steps = (table->table1->size + table->table2->size) / 2;

	// choose table 2 as first to try if it has less keys than table 1,
	// else choose table 1 as first table to try
	InnerTable *inner_table;
	if (table->table2->nkeys < table->table1->nkeys) {
		cur_table_num = 2;
	} else {
		cur_table_num = 1;
	}

	while (key) {
		// setup values for current iteration
		if (cur_table_num == 1) {
			cur_table = table->table1;
			hash = h1(key);
		} else {
			cur_table = table->table2;
			hash = h2(key);
		}
		address = rightmostnbits(inner_table->depth, hash);

		// if been cuckooing too long need to split bucket, potentially
		// doubling table size
		if (steps >= max_steps) {
			split_bucket(table, address);
			max_steps = (table->table1->size + table->table2->size) / 2;
		}

		// if destination slot is occupied need save it's val before moving on
		// so we can rehash it. else prepare loop to break and cur_table to
		// get the empty slot occupied
		if (cur_table->buckets[address]->full &&
		    cur_table->buckets[address]->key == key) {
			next_key = cur_table->key;
		} else {
			cur_table->buckets[address]->full = true;
			cur_table->nkeys += 1;
			next_key = false;
		}

		// insert key into it's desired slot
		cur_table->buckets[address] = key;

		// set key to next key (if any)
		key = next_key;

		// alternate between inserting into table 1 and 2
		cur_table_num = (cur_table_num == 1) ? 2: 1;

		// increment step counter
		steps += 1;
	}


	return true;
}


// lookup whether 'key' is inside 'table'
// returns true if found, false if not
bool xuckoo_hash_table_lookup(XuckooHashTable *table, int64 key) {
	assert(table);
	int h;

	// calculate the address for this key in table 1
	int address = rightmostnbits(table->table1->depth, h1(key));

	// check if key in table 1
	if (table->table1->buckets[address]->full &&
		table->table1->buckets[address] == key) {
		return true;
	}

	// calculate the address for this key in table 2
	address = rightmostnbits(table->table2->depth, h2(key));

	// check if key in table 2
	if (table->table2->buckets[address]->full &&
		table->table2->buckets[address] == key) {
		return true;
	}

	// key is in neither of the tables
	return false;
}


// print the contents of 'table' to stdout
void xuckoo_hash_table_print(XuckooHashTable *table) {
	assert(table != NULL);

	printf("--- table ---\n");

	// loop through the two tables, printing them
	InnerTable *innertables[2] = {table->table1, table->table2};
	int t;
	for (t = 0; t < 2; t++) {
		// print header
		printf("table %d\n", t+1);

		printf("  table:               buckets:\n");
		printf("  address | bucketid   bucketid [key]\n");

		// print table and buckets
		int i;
		for (i = 0; i < innertables[t]->size; i++) {
			// table entry
			printf("%9d | %-9d ", i, innertables[t]->buckets[i]->id);

			// if this is the first address at which a bucket occurs, print it
			if (innertables[t]->buckets[i]->id == i) {
				printf("%9d ", innertables[t]->buckets[i]->id);
				if (innertables[t]->buckets[i]->full) {
					printf("[%llu]", innertables[t]->buckets[i]->key);
				} else {
					printf("[ ]");
				}
			}

			// end the line
			printf("\n");
		}
	}
	printf("--- end table ---\n");
}


// print some statistics about 'table' to stdout
void xuckoo_hash_table_stats(XuckooHashTable *table) {
	fprintf(stderr, "not yet implemented\n");
	return;
}
