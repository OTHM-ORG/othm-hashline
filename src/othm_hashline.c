/*    This file is part of Othm_hashline.
 *
 *    Othm_hashline is free software: you can redistribute it and/or modify
 *    it under the terms of the Lesser GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Othm_hashmap is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Lesser GNU General Public License for more details.
 *
 *    You should have received a copy of the Lesser GNU General Public License
 *    along with Ruspma.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MurmurHash2.h"
#include "othm_hashline.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define DEFAULT_ENTRIES_TO_HASHBINS 1
#define DEFAULT_HASH_SEED 37

const static int hashline_primes[] = {
	256 + 27, 512 + 9, 1024 + 9, 2048 + 5, 4096 + 3, 8192 + 27,
	16384 + 43, 32768 + 3, 65536 + 45, 131072 + 29, 262144 + 3,
	524288 + 21, 1048576 + 7, 2097152 + 17, 4194304 + 15,
	8388608 + 9, 16777216 + 43, 33554432 + 35, 67108864 + 15,
	134217728 + 29, 268435456 + 3, 536870912 + 11, 1073741824 + 85, 0
};

static void rehash(struct othm_hashline *);

/* Copies value of request pointer into the hashbin */
static struct othm_hashline_entry *new_hashentry(struct othm_request *request)
{
	struct othm_hashline_entry *new_hashentry;

	new_hashentry = malloc(sizeof(struct othm_hashline_entry));
	new_hashentry->key = request;
	new_hashentry->next = NULL;
	return new_hashentry;
}

/* frees hashbin without freeing request, used in rehashing */
static void hashentry_free(struct othm_hashline_entry *hashentry)
{
	free(hashentry);
}

/* creates a new hashline */
struct othm_hashline *othm_hashline_new(struct othm_hashline *(*gen)(void))
{
	struct othm_hashline *new_hashline;
	struct othm_hashline_bin *hashbin_ptr;
	int i;

	if (gen != NULL)
		new_hashline = gen();
	else
		new_hashline = malloc(sizeof(struct othm_hashline));
	new_hashline->hashbin_num = hashline_primes[0];
	new_hashline->hashbins =
		malloc(sizeof(struct othm_hashline_bin) *
		       hashline_primes[0]);
	new_hashline->entries_num = 0;
	new_hashline->primes_pointer = hashline_primes;
	hashbin_ptr = new_hashline->hashbins;
	for (i = 0; i != hashline_primes[0]; ++i) {
		hashbin_ptr->first = NULL;
		++hashbin_ptr;
	}
	return new_hashline;
}

/* Avoid uneeded rehashings by setting up the hashline at creation
   with a point in the sequence */
struct othm_hashline *othm_hashline_new_seq(struct othm_hashline *(*gen)(void),
					    int sequence)
{
	struct othm_hashline *new_hashline;
	struct othm_hashline_bin *hashbin_ptr;
	int i;

	new_hashline = gen();
	new_hashline->hashbin_num = hashline_primes[sequence];
	new_hashline->hashbins =
		malloc(sizeof(struct othm_hashline_bin) *
		       hashline_primes[sequence]);
	new_hashline->entries_num = 0;
	new_hashline->primes_pointer = hashline_primes + sequence;
	hashbin_ptr = new_hashline->hashbins;
	for (i = 0; i != hashline_primes[sequence]; ++i) {
		hashbin_ptr->first = NULL;
		++hashbin_ptr;
	}
	return new_hashline;
}

/* Final freeing of hashline */
void othm_hashline_free(struct othm_hashline *hashline,
			void (*map_free)(struct othm_hashline *map))
{
	struct othm_hashline_bin *current_hashbin;
	unsigned int hashbin_num;
	unsigned int i;

	current_hashbin = hashline->hashbins;
	hashbin_num = hashline->hashbin_num;
	for (i = 0; i != hashbin_num; ++i) {
		struct othm_hashline_entry *current_hashentry;

		current_hashentry = current_hashbin->first;
		while (current_hashentry != NULL) {
			struct othm_hashline_entry *current_hashentry_holder;

			current_hashentry_holder = current_hashentry;
			current_hashentry = current_hashentry->next;
			hashentry_free(current_hashentry_holder);
		}
		++current_hashbin;
	}
	free(hashline->hashbins);
	if (map_free != NULL)
		map_free(hashline);
	else
		free(hashline);
}

/* checks to see if hashentry uses request*/
static int check_request_hashentry(struct othm_hashline_entry *hashentry,
				   struct othm_request *request)
{
	if (hashentry->key->key_type != request->key_type)
		return 0;
	return request->check_key(hashentry->key->data, request->data);
}

/* Adds an element to the hashline */
int othm_hashline_add(struct othm_hashline *hashline,
		      struct othm_request *request)
{
	struct othm_hashline_entry *hashentry;
	unsigned int row;

	if (hashline->entries_num / hashline->hashbin_num >=
	    DEFAULT_ENTRIES_TO_HASHBINS)
		rehash(hashline);

	row = MurmurHash2(request->data, request->data_size, DEFAULT_HASH_SEED)
		% hashline->hashbin_num;
        hashentry = hashline->hashbins[row].first;

	if(hashentry == NULL) {
		hashline->hashbins[row].first = new_hashentry(request);
		++hashline->entries_num;
		return OTHM_HASHLINE_ADDED;
	}

	if(check_request_hashentry(hashentry, request))
		return OTHM_HASHLINE_ALREADY_PRESENT;


	while(hashentry->next != NULL) {
		hashentry = hashentry->next;
		if(check_request_hashentry(hashentry, request))
			return OTHM_HASHLINE_ALREADY_PRESENT;
	}
	hashentry->next = new_hashentry(request);
	++hashline->entries_num;
	return OTHM_HASHLINE_ADDED;
}

void othm_hashline_remove(struct othm_hashline *hashline,
			  struct othm_request *request)
{
	struct othm_hashline_entry *hashentry;
	unsigned int row;

	row = MurmurHash2(request->data, request->data_size, DEFAULT_HASH_SEED)
		% hashline->hashbin_num;
        hashentry = hashline->hashbins[row].first;

	if(hashentry == NULL)
		return;

	if(check_request_hashentry(hashentry, request)) {
		hashline->hashbins[row].first = hashentry->next;
		hashentry_free(hashentry);
		--hashline->entries_num;
		return;
	}


	while(hashentry->next != NULL) {
		struct othm_hashline_entry *next = hashentry->next;
		if(check_request_hashentry(next, request)) {
			hashentry->next = next->next;
			hashentry_free(next);
			--hashline->entries_num;
			return;
		}
		hashentry = next;
	}
}

/* Gets an element from a hashline */
int othm_hashline_get(struct othm_hashline *hashline,
		      struct othm_request *request)
{
	struct othm_hashline_entry *hashentry;
	unsigned int row;

        row = MurmurHash2(request->data, request->data_size, DEFAULT_HASH_SEED)
		% hashline->hashbin_num;

	hashentry = hashline->hashbins[row].first;
	while (hashentry != NULL) {
		if(check_request_hashentry(hashentry, request))
			return 1;
		hashentry = hashentry->next;
	}
	return 0;
}

/* Adds to a bin something already in the hashline during a rehash */
static void rehash_add(struct othm_hashline_bin *hashbin,
		       struct othm_hashline_entry *adding_hashentry)
{
	struct othm_hashline_entry *current_hashentry;

        current_hashentry = hashbin->first;
	adding_hashentry->next = NULL;
	if (current_hashentry == NULL) {
		hashbin->first = adding_hashentry;
		return;
	}
	while (current_hashentry->next != NULL)
		current_hashentry = current_hashentry->next;
	current_hashentry->next = adding_hashentry;
}

/* Rehashes the hashline */
static void rehash(struct othm_hashline *hashline)
{
	struct othm_hashline_bin *old_hashbins;
	struct othm_hashline_bin *new_hashbins;
	struct othm_hashline_bin *current_hashbin;
	unsigned int old_hashbin_num;
	unsigned int new_hashbin_num;
	unsigned int i;

	old_hashbin_num = hashline->hashbin_num;
	new_hashbin_num = hashline->primes_pointer[1];

	old_hashbins = hashline->hashbins;
	new_hashbins = malloc(sizeof(struct othm_hashline_bin) *
			      new_hashbin_num);

	current_hashbin = old_hashbins;
	for (i = 0; i != old_hashbin_num; ++i) {
		struct othm_hashline_entry *current_hashentry;

		current_hashentry = current_hashbin->first;
		while (current_hashentry != NULL) {
			struct othm_hashline_entry *current_hashentry_holder;
			unsigned int row;

			current_hashentry_holder = current_hashentry;
			current_hashentry = current_hashentry->next;
			row = MurmurHash2(current_hashentry_holder->key->data,
					  current_hashentry_holder->key->data_size,
					  DEFAULT_HASH_SEED) % new_hashbin_num;
			rehash_add(new_hashbins + row, current_hashentry_holder);
		}
		++current_hashbin;
	}
	free(hashline->hashbins);
	hashline->hashbins = new_hashbins;
	++hashline->primes_pointer;
	hashline->hashbin_num = new_hashbin_num;
}
