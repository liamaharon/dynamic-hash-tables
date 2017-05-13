/* * * * * * * * *
 * Dynamic hash table using extendible hashing with multiple keys per bucket,
 * resolving collisions by incrementally growing the hash table
 *
 * created for COMP20007 Design of Algorithms - Assignment 2, 2017
 * by Liam Aharon
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>

#include "xtndbln.h"

// macro to calculate the rightmost n bits of a number x
#define rightmostnbits(n, x) (x) & ((1 << (n)) - 1)

// a bucket stores an array of keys
// it also knows how many bits are shared between possible keys, and the first
// table address that references it
typedef struct xtndbln_bucket {
	int id;			// a unique id for this bucket, equal to the first address
					// in the table which points to it
	int depth;		// how many hash value bits are being used by this bucket
	int nkeys;		// number of keys currently contained in this bucket
	int64 *keys;	// the keys stored in this bucket
} Bucket;

// helper structure to store statistics gathered
typedef struct stats {
	int nbuckets;	// how many distinct buckets does the table point to
	int nkeys;		// how many keys are being stored in the table
	int time;		// how much CPU time has been used to insert/lookup keys
					// in this table
} Stats;

// a hash table is an array of slots pointing to buckets holding up to
// bucketsize keys, along with some information about the number of hash value
// bits to use for addressing
struct xtndbln_table {
	Bucket **buckets;	// array of pointers to buckets
	int size;			// how many entries in the table of pointers (2^depth)
	int depth;			// how many bits of the hash value to use (log2(size))
	int bucketsize;		// maximum number of keys per bucket
	Stats stats;
};

/* * * *
 * helper functions
 */

 // create a new bucket first referenced from 'first_address', based on 'depth'
 // bits of its keys' hash values
 Bucket *new_bucket(int first_address, int depth, int bucketsize) {
 	Bucket *bucket = malloc(sizeof *bucket);
 	assert(bucket);

	// setup array of keys
	bucket->keys = malloc((sizeof *bucket->keys) * bucketsize);
	assert(bucket->keys);
	bucket->nkeys = 0;

	bucket->id = first_address;
	bucket->depth = depth;

	return bucket;
 }

// set this up after getting table initialised
 // double the table of bucket pointers, duplicating the bucket pointers in the
 // first half into the new second half of the table
static void double_table(XtndblNHashTable *table) {
	int size = table->size * 2;
	assert(size < MAX_TABLE_SIZE && "error: table has grown too large!");

	// get a new array of twice as many bucket pointers, and copy pointers down
	table->buckets = realloc(table->buckets, (sizeof *table->buckets) * size);
	assert(table->buckets);
	int i;
	for (i = 0; i < table->size; i++) {
		table->buckets[table->size + i] = table->buckets[i];
	}

	// finally, increase the table size and the depth we are using to hash keys
	table->size = size;
	table->depth++;
}

// need to set this up, searching down the array for a place to store the key
// reinsert a key into the hash table after splitting a bucket --- we can assume
// that there will definitely be space for this key because it was already
// inside the hash table previously
// use 'xtndbl1_hash_table_insert()' instead for inserting new keys
static void reinsert_key(XtndblNHashTable *table, int64 key) {
	int address = rightmostnbits(table->depth, h1(key));

	// point to insert into
	int insersion_point = table->buckets[address]->nkeys;

	// insert into next point in bucket
	table->buckets[address]->keys[insersion_point] = key;
	table->buckets[address]->nkeys += 1;
}

// need to set this up eventually, changing lsat bit where keys is reinserted
// to reinsert every key from that bucket
 // split the bucket in 'table' at address 'address', growing table if necessary
static void split_bucket(XtndblNHashTable *table, int address) {
	assert(table);

	// FIRST,
	// do we need to grow the table?
	if (table->buckets[address]->depth == table->depth) {
		// yep, this bucket is down to its last pointer
		double_table(table);
	}
	// either way, now it's time to split this bucket

	// SECOND,
	// create a new bucket and update both buckets' depth
	Bucket *bucket = table->buckets[address];
	int depth = bucket->depth;
	int first_address = bucket->id;
	int bucketsize = table->bucketsize;

	int new_depth = depth + 1;
	bucket->depth = new_depth;

	// new bucket's first address will be a 1 bit plus the old first address
	int new_first_address = 1 << depth | first_address;
	Bucket *newbucket = new_bucket(new_first_address, new_depth, bucketsize);
	table->stats.nbuckets++;

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
	int maxprefix = 1 << (table->depth - new_depth);

	int prefix;
	for (prefix = 0; prefix < maxprefix; prefix++) {

		// construct address by joining this prefix and the suffix
		int a = (prefix << new_depth) | suffix;

		// redirect this table entry to point at the new bucket
		table->buckets[a] = newbucket;
	}

	// FINALLY,
	// filter the keys from the old bucket into their rightful place in the new
	// table (which may be the old bucket, or may be the new bucket)

	// make a copy of the keys
	int64 *tmp_keys = malloc((sizeof *tmp_keys) * bucket->nkeys);
	memcpy(tmp_keys, bucket->keys, bucket->nkeys * sizeof(int64));
	int nkeys = bucket->nkeys;

	// set bucket as empty
	bucket->nkeys = 0;

	// reinsert the keys into the table
	int i;
	for (i=0; i<nkeys; i++) {
		reinsert_key(table, tmp_keys[i]);
	}
	free(tmp_keys);
}

 /* * * *
  * all functions
  */

// initialise an extendible hash table with 'bucketsize' keys per bucket
XtndblNHashTable *new_xtndbln_hash_table(int bucketsize) {
	XtndblNHashTable *table = malloc(sizeof *table);
	assert(table);

	table->size = 1;

	table->bucketsize = bucketsize;
	table->buckets = malloc((sizeof *table->buckets) * table->size);
	assert(table->buckets);
	table->buckets[0] = new_bucket(0, 0, bucketsize);

	table->depth = 0;

	table->stats.nbuckets = 1;
	table->stats.nkeys = 0;
	table->stats.time = 0;

	return table;
}


// free all memory associated with 'table'
void free_xtndbln_hash_table(XtndblNHashTable *table) {
	assert(table);

	// loop backwards through the array of pointers, freeing buckets only as we
	// reach their first reference
	// (if we loop through forwards, we wouldn't know which reference was last)
	int i;
	for (i = table->size-1; i >= 0; i--) {
		if (table->buckets[i]->id == i) {
			free(table->buckets[i]->keys);
			free(table->buckets[i]);
		}
	}

	// free the array of bucket pointers
	free(table->buckets);

	// free the table struct itself
	free(table);
}


// insert 'key' into 'table', if it's not in there already
// returns true if insertion succeeds, false if it was already in there
bool xtndbln_hash_table_insert(XtndblNHashTable *table, int64 key) {
	assert(table);
	int start_time = clock(); // start timing

	// calculate table address
	int hash = h1(key);
	int address = rightmostnbits(table->depth, hash);

	// check if key already in table
	if (xtndbln_hash_table_lookup(table, key) == true) {
		table->stats.time += clock() - start_time; // add time elapsed
		return false;
	};

	// if not, make space in the table until our target bucket has space
	while (table->buckets[address]->nkeys == table->bucketsize) {
		split_bucket(table, address);
		// recalculate address because we might now need more bits
		address = rightmostnbits(table->depth, hash);
	}

	// there's now space! we can insert this key at the next avaliable position
	// in the bucket, record time and return
	int nkeys = table->buckets[address]->nkeys += 1;
	table->buckets[address]->keys[nkeys-1] = key;
	table->stats.nkeys++;
	table->stats.time += clock() - start_time;
	return true;
}


// lookup whether 'key' is inside 'table'
// returns true if found, false if not
bool xtndbln_hash_table_lookup(XtndblNHashTable *table, int64 key) {
	assert(table);
	int start_time = clock(); // start timing
	int i;

	// calculate table address for this key
	int address = rightmostnbits(table->depth, h1(key));

	// check if destination bucket is occupied
	if (table->buckets[address]->nkeys > 0) {
		// search bucket
		for (i=0; i<table->buckets[address]->nkeys; i++) {
			// if found return record time and true
			if (table->buckets[address]->keys[i] == key) {
				table->stats.time += clock() - start_time;
				return true;
			}
		}
	}
	// not found, record time and return false
	table->stats.time += clock() - start_time;
	return false;
}


// print the contents of 'table' to stdout
void xtndbln_hash_table_print(XtndblNHashTable *table) {
	assert(table);
	printf("--- table size: %d\n", table->size);

	// print header
	printf("  table:               buckets:\n");
	printf("  address | bucketid   bucketid [key]\n");

	// print table and buckets
	int i;
	for (i = 0; i < table->size; i++) {
		// table entry
		printf("%9d | %-9d ", i, table->buckets[i]->id);

		// if this is the first address at which a bucket occurs, print it now
		if (table->buckets[i]->id == i) {
			printf("%9d ", table->buckets[i]->id);

			// print the bucket's contents
			printf("[");
			for(int j = 0; j < table->bucketsize; j++) {
				if (j < table->buckets[i]->nkeys) {
					printf(" %llu", table->buckets[i]->keys[j]);
				} else {
					printf(" -");
				}
			}
			printf(" ]");
		}
		// end the line
		printf("\n");
	}

	printf("--- end table ---\n");
}


// print some statistics about 'table' to stdout
void xtndbln_hash_table_stats(XtndblNHashTable *table) {
	assert(table);

	printf("--- table stats ---\n");

	// print some stats about state of the table
	printf("current table size: %d\n", table->size);
	printf("    number of keys: %d\n", table->stats.nkeys);
	printf(" number of buckets: %d\n", table->stats.nbuckets);

	// also calculate CPU usage in seconds and print this
	float seconds = table->stats.time * 1.0 / CLOCKS_PER_SEC;
	printf("    CPU time spent: %.6f sec\n", seconds);

	printf("--- end stats ---\n");
}
