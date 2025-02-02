/*
 * Copyright 2008  Veselin Georgiev,
 * anrieffNOSPAM @ mgail_DOT.com (convert to gmail)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "libcpuid.h"
#include "libcpuid_util.h"

void match_features(const struct feature_map_t* matchtable, int count, uint32_t reg, struct cpu_id_t* data)
{
	int i;
	for (i = 0; i < count; i++)
		if (reg & (1u << matchtable[i].bit))
			data->flags[matchtable[i].feature] = 1;
}

#ifndef HAVE_POPCOUNT64
static unsigned int popcount64(uint64_t mask)
{
	unsigned int num_set_bits = 0;

	while (mask) {
		mask &= mask - 1;
		num_set_bits++;
	}

	return num_set_bits;
}
#endif

static int score(const struct match_entry_t* entry, const struct cpu_id_t* data,
                 int brand_code, uint64_t bits, int model_code)
{
	int res = 0;
	if (entry->family	== data->family    ) res += 2;
	if (entry->model	== data->model     ) res += 2;
	if (entry->stepping	== data->stepping  ) res += 2;
	if (entry->ext_family	== data->ext_family) res += 2;
	if (entry->ext_model	== data->ext_model ) res += 2;
	if (entry->ncores	== data->num_cores ) res += 2;
	if (entry->l2cache	== data->l2_cache  ) res += 1;
	if (entry->l3cache	== data->l3_cache  ) res += 1;
	if (entry->brand_code   == brand_code  ) res += 2;
	if (entry->model_code   == model_code  ) res += 2;

	res += popcount64(entry->model_bits & bits) * 2;
	return res;
}

int match_cpu_codename(const struct match_entry_t* matchtable, int count,
                       struct cpu_id_t* data, int brand_code, uint64_t bits,
                       int model_code)
{
	int bestscore = -1;
	int bestindex = 0;
	int i, t;

	for (i = 0; i < count; i++) {
		t = score(&matchtable[i], data, brand_code, bits, model_code);
		if (t > bestscore) {
			bestscore = t;
			bestindex = i;
		}
	}
	strcpy_s(data->cpu_codename, sizeof(data->cpu_codename), matchtable[bestindex].name);
	return bestscore;
}

static int xmatch_entry(char c, const char* p)
{
	int i, j;
	if (c == 0) return -1;
	if (c == p[0]) return 1;
	if (p[0] == '.') return 1;
	if (p[0] == '#' && isdigit(c)) return 1;
	if (p[0] == '[') {
		j = 1;
		while (p[j] && p[j] != ']') j++;
		if (!p[j]) return -1;
		for (i = 1; i < j; i++)
			if (p[i] == c) return j + 1;
	}
	return -1;
}

int match_pattern(const char* s, const char* p)
{
	int i, j, dj, k, n, m;
	n = (int) strlen(s);
	m = (int) strlen(p);
	for (i = 0; i < n; i++) {
		if (xmatch_entry(s[i], p) != -1) {
			j = 0;
			k = 0;
			while (j < m && ((dj = xmatch_entry(s[i + k], p + j)) != -1)) {
				k++;
				j += dj;
			}
			if (j == m) return i + 1;
		}
	}
	return 0;
}

struct cpu_id_t* get_cached_cpuid(void)
{
	static int initialized = 0;
	static struct cpu_id_t id;
	if (initialized) return &id;
	if (cpu_identify(NULL, &id))
		memset(&id, 0, sizeof(id));
	initialized = 1;
	return &id;
}

int match_all(uint64_t bits, uint64_t mask)
{
	return (bits & mask) == mask;
}

/* Functions to manage cpu_affinity_mask_t type
 * Adapted from https://electronics.stackexchange.com/a/200070
 */
void init_affinity_mask(cpu_affinity_mask_t *affinity_mask)
{
	memset(affinity_mask->__bits, 0x00, __MASK_SETSIZE);
}

void copy_affinity_mask(cpu_affinity_mask_t *dest_affinity_mask, cpu_affinity_mask_t *src_affinity_mask)
{
	memcpy(dest_affinity_mask->__bits, src_affinity_mask->__bits, __MASK_SETSIZE);
}

void set_affinity_mask_bit(logical_cpu_t logical_cpu, cpu_affinity_mask_t *affinity_mask)
{
	affinity_mask->__bits[logical_cpu / __MASK_NCPUBITS] |= 0x1 << (logical_cpu % __MASK_NCPUBITS);
}

bool get_affinity_mask_bit(logical_cpu_t logical_cpu, cpu_affinity_mask_t *affinity_mask)
{
	return (affinity_mask->__bits[logical_cpu / __MASK_NCPUBITS] & (0x1 << (logical_cpu % __MASK_NCPUBITS))) != 0x00;
}

void clear_affinity_mask_bit(logical_cpu_t logical_cpu, cpu_affinity_mask_t *affinity_mask)
{
	affinity_mask->__bits[logical_cpu / __MASK_NCPUBITS] &= ~(0x1 << (logical_cpu % __MASK_NCPUBITS));
}
