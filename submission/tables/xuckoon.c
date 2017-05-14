/* * * * * * * * *
* Dynamic hash table using a combination of extendible hashing and cuckoo
* hashing with a single keys per bucket, resolving collisions by switching keys
* between two tables with two separate hash functions and growing the tables
* incrementally in response to cycles
*
* created for COMP20007 Design of Algorithms - Assignment 2, 2017
* by Liam Aharon
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "xuckoon.h"

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
	int nbuckets;		// how many buckets are being stored in the table
} InnerTable;

// a xuckoo hash table is just two inner tables for storing inserted keys
struct xuckoon_table {
	InnerTable *table1;
	InnerTable *table2;
	int time;          // how much CPU time has been used to insert/lookup keys
					   // in this table
};


/* * * *
 * helper functions
 */

 // create a new bucket first referenced from 'first_address', based on 'depth'
 // bits of its keys' hash values
// static Bucket *new_bucket(int first_address, int depth) {
// 	Bucket *bucket=NULL;
// 	return bucket;
// }
//
//
// // double the table of bucket pointers, duplicating the bucket pointers in the
// // first half into the new second half of the table
// static void double_table(InnerTable *inner_table) {
//
// }
//
//
// // split the bucket in 'table' table_num at address 'address', growing table
// // if necessary
// static void split_bucket(InnerTable *inner_table, int address, int table_num) {
//
// }
//
//
// // init a new inner_table
// static InnerTable *new_inner_table() {
// 	InnerTable *inner_table=NULL;
// 	return inner_table;
//  }

// // free all memory associated with the inner table
// static void free_inner_table(InnerTable *inner_table) {
//
// }

/* * * *
 * all functions
 */

// initialise an extendible cuckoo hash table
XuckoonHashTable *new_xuckoon_hash_table() {
	XuckoonHashTable *table=NULL;
	return table;
}




// free all memory associated with 'table'
void free_xuckoon_hash_table(XuckoonHashTable *table) {

}


// insert 'key' into 'table', if it's not in there already
// returns true if insertion succeeds, false if it was already in there
bool xuckoon_hash_table_insert(XuckoonHashTable *table, int64 key) {

	return true;
}


// lookup whether 'key' is inside 'table'
// returns true if found, false if not
bool xuckoon_hash_table_lookup(XuckoonHashTable *table, int64 key) {

	return false;
}


// print the contents of 'table' to stdout
void xuckoon_hash_table_print(XuckoonHashTable *table) {
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
void xuckoon_hash_table_stats(XuckoonHashTable *table) {
	assert(table);

	InnerTable *table1 = table->table1;
	InnerTable *table2 = table->table2;

	// compute some stats
	int total_keys = table1->nkeys + table2->nkeys;
	int total_buckets = table1->nbuckets + table2->nbuckets;
	float t1_load_factor = table1->nbuckets * 100.0 / table1->size;
	float t2_load_factor = table2->nbuckets * 100.0 / table2->size;
	// calculate the % of keys and buckets each table holds
	float t1_keyp = table1->nkeys * 100.0 / total_keys;
	float t2_keyp = table2->nkeys * 100.0 / total_keys;
	float t1_bucketp = table1->nbuckets * 100.0 / total_buckets;
	float t2_bucketp = table2->nbuckets * 100.0 / total_buckets;

	printf("--- table stats ---\n");

	// print some stats about state of the table
	printf("table 1:\n");
	printf("    %d slots\n", table1->size);
	printf("    %d keys\n", table1->nkeys);
	printf("    %d buckets\n", table1->nbuckets);
	printf("    %.1f%% of all keys\n", t1_keyp);
	printf("    %.1f%% of all buckets\n", t1_bucketp);
	printf("    load factor of %.3f%% (nbuckets/slots)\n", t1_load_factor);
	printf("table 2:\n");
	printf("    %d slots\n", table2->size);
	printf("    %d keys\n", table2->nkeys);
	printf("    %d buckets\n", table2->nbuckets);
	printf("    %.1f%% of all keys\n", t2_keyp);
	printf("    %.1f%% of all buckets\n", t2_bucketp);
	printf("    load factor of %.3f%% (nbuckets/slots)\n", t2_load_factor);

	// also calculate CPU usage in seconds and print this
	float seconds = table->time * 1.0 / CLOCKS_PER_SEC;
	printf("CPU time spent: %.6f sec\n", seconds);

	printf("--- end stats ---\n");
}
