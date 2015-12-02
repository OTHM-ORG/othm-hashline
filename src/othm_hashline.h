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

#ifndef OTHM_HASHLINE_H
#define OTHM_HASHLINE_H

#include <othm_base.h>

enum { OTHM_HASHLINE_ALREADY_PRESENT, OTHM_HASHLINE_ADDED };

struct othm_hashline_entry {
	struct othm_request *key;
	struct othm_hashline_entry *next;
};

struct othm_hashline_bin {
	struct othm_hashline_entry *first;
};

struct othm_hashline {
	unsigned int hashbin_num;
	int entries_num;
	const int *primes_pointer;
	struct othm_hashline_bin *hashbins;
};

#define OTHMHASHLINE(HASHLINE) ((struct othm_hashline *) (HASHLINE))

struct othm_hashline *othm_hashline_new_seq(struct othm_hashline *(*gen)(void),
					    int seq);

struct othm_hashline *othm_hashline_new(struct othm_hashline *(*gen)(void));

void othm_hashline_free(struct othm_hashline *hashline,
			void (*line_free)(struct othm_hashline *line));

int othm_hashline_add(struct othm_hashline *hashline,
		      struct othm_request *request);

void othm_hashline_remove(struct othm_hashline *hashline,
			  struct othm_request *request);

int othm_hashline_get(struct othm_hashline *hashline,
		      struct othm_request *request);

#endif
