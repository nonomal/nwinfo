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
#include "libcpuid.h"
#include "libcpuid_internal.h"
#include "recog_intel.h"
#include "recog_amd.h"
#include "asm-bits.h"
#include "libcpuid_util.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Implementation: */

static int _libcpiud_errno = ERR_OK;

int set_error(cpu_error_t err)
{
	_libcpiud_errno = (int) err;
	return (int) err;
}

static void raw_data_t_constructor(struct cpu_raw_data_t* raw)
{
	memset(raw, 0, sizeof(struct cpu_raw_data_t));
}

static void cpu_id_t_constructor(struct cpu_id_t* id)
{
	memset(id, 0, sizeof(struct cpu_id_t));
	id->l1_data_cache = id->l1_instruction_cache = id->l2_cache = id->l3_cache = id->l4_cache = -1;
	id->l1_assoc = id->l1_data_assoc = id->l1_instruction_assoc = id->l2_assoc = id->l3_assoc = id->l4_assoc = -1;
	id->l1_cacheline = id->l1_data_cacheline = id->l1_instruction_cacheline = id->l2_cacheline = id->l3_cacheline = id->l4_cacheline = -1;
	id->sse_size = -1;
}

static int parse_token(const char* expected_token, const char *token,
                        const char *value, uint32_t array[][4], int limit, int *recognized)
{
	char format[32];
	int veax, vebx, vecx, vedx;
	int index;

	if (*recognized) return 1; /* already recognized */
	if (strncmp(token, expected_token, strlen(expected_token))) return 1; /* not what we search for */
	sprintf(format, "%s[%%d]", expected_token);
	*recognized = 1;
	if (1 == sscanf(token, format, &index) && index >=0 && index < limit) {
		if (4 == sscanf(value, "%x%x%x%x", &veax, &vebx, &vecx, &vedx)) {
			array[index][0] = veax;
			array[index][1] = vebx;
			array[index][2] = vecx;
			array[index][3] = vedx;
			return 1;
		}
	}
	return 0;
}

/* get_total_cpus() system specific code: uses OS routines to determine total number of CPUs */
static int get_total_cpus(void)
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return system_info.dwNumberOfProcessors;
}

static void load_features_common(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	const struct feature_map_t matchtable_edx1[] = {
		{  0, CPU_FEATURE_FPU },
		{  1, CPU_FEATURE_VME },
		{  2, CPU_FEATURE_DE },
		{  3, CPU_FEATURE_PSE },
		{  4, CPU_FEATURE_TSC },
		{  5, CPU_FEATURE_MSR },
		{  6, CPU_FEATURE_PAE },
		{  7, CPU_FEATURE_MCE },
		{  8, CPU_FEATURE_CX8 },
		{  9, CPU_FEATURE_APIC },
		{ 11, CPU_FEATURE_SEP },
		{ 12, CPU_FEATURE_MTRR },
		{ 13, CPU_FEATURE_PGE },
		{ 14, CPU_FEATURE_MCA },
		{ 15, CPU_FEATURE_CMOV },
		{ 16, CPU_FEATURE_PAT },
		{ 17, CPU_FEATURE_PSE36 },
		{ 19, CPU_FEATURE_CLFLUSH },
		{ 23, CPU_FEATURE_MMX },
		{ 24, CPU_FEATURE_FXSR },
		{ 25, CPU_FEATURE_SSE },
		{ 26, CPU_FEATURE_SSE2 },
		{ 28, CPU_FEATURE_HT },
	};
	const struct feature_map_t matchtable_ecx1[] = {
		{  0, CPU_FEATURE_PNI },
		{  1, CPU_FEATURE_PCLMUL },
		{  3, CPU_FEATURE_MONITOR },
		{  9, CPU_FEATURE_SSSE3 },
		{ 12, CPU_FEATURE_FMA3 },
		{ 13, CPU_FEATURE_CX16 },
		{ 19, CPU_FEATURE_SSE4_1 },
		{ 20, CPU_FEATURE_SSE4_2 },
		{ 22, CPU_FEATURE_MOVBE },
		{ 23, CPU_FEATURE_POPCNT },
		{ 25, CPU_FEATURE_AES },
		{ 26, CPU_FEATURE_XSAVE },
		{ 27, CPU_FEATURE_OSXSAVE },
		{ 28, CPU_FEATURE_AVX },
		{ 29, CPU_FEATURE_F16C },
		{ 30, CPU_FEATURE_RDRAND },
	};
	const struct feature_map_t matchtable_ebx7[] = {
		{  3, CPU_FEATURE_BMI1 },
		{  5, CPU_FEATURE_AVX2 },
		{  8, CPU_FEATURE_BMI2 },
		{ 18, CPU_FEATURE_RDSEED },
		{ 19, CPU_FEATURE_ADX },
		{ 29, CPU_FEATURE_SHA_NI },
	};
	const struct feature_map_t matchtable_edx81[] = {
		{ 11, CPU_FEATURE_SYSCALL },
		{ 27, CPU_FEATURE_RDTSCP },
		{ 29, CPU_FEATURE_LM },
	};
	const struct feature_map_t matchtable_ecx81[] = {
		{  0, CPU_FEATURE_LAHF_LM },
		{  5, CPU_FEATURE_ABM },
	};
	const struct feature_map_t matchtable_edx87[] = {
		{  8, CPU_FEATURE_CONSTANT_TSC },
	};
	if (raw->basic_cpuid[0][EAX] >= 1) {
		match_features(matchtable_edx1, COUNT_OF(matchtable_edx1), raw->basic_cpuid[1][EDX], data);
		match_features(matchtable_ecx1, COUNT_OF(matchtable_ecx1), raw->basic_cpuid[1][ECX], data);
	}
	if (raw->basic_cpuid[0][EAX] >= 7) {
		match_features(matchtable_ebx7, COUNT_OF(matchtable_ebx7), raw->basic_cpuid[7][EBX], data);
	}
	if (raw->ext_cpuid[0][EAX] >= 0x80000001) {
		match_features(matchtable_edx81, COUNT_OF(matchtable_edx81), raw->ext_cpuid[1][EDX], data);
		match_features(matchtable_ecx81, COUNT_OF(matchtable_ecx81), raw->ext_cpuid[1][ECX], data);
	}
	if (raw->ext_cpuid[0][EAX] >= 0x80000007) {
		match_features(matchtable_edx87, COUNT_OF(matchtable_edx87), raw->ext_cpuid[7][EDX], data);
	}
	if (data->flags[CPU_FEATURE_SSE]) {
		/* apply guesswork to check if the SSE unit width is 128 bit */
		switch (data->vendor) {
			case VENDOR_AMD:
				data->sse_size = (data->ext_family >= 16 && data->ext_family != 17) ? 128 : 64;
				break;
			case VENDOR_INTEL:
				data->sse_size = (data->family == 6 && data->ext_model >= 15) ? 128 : 64;
				break;
			default:
				break;
		}
		/* leave the CPU_FEATURE_128BIT_SSE_AUTH 0; the advanced per-vendor detection routines
		 * will set it accordingly if they detect the needed bit */
	}
}

static cpu_vendor_t cpuid_vendor_identify(const uint32_t *raw_vendor, char *vendor_str)
{
	int i;
	cpu_vendor_t vendor = VENDOR_UNKNOWN;
	const struct { cpu_vendor_t vendor; char match[16]; }
	matchtable[NUM_CPU_VENDORS] = {
		/* source: http://www.sandpile.org/ia32/cpuid.htm */
		{ VENDOR_INTEL		, "GenuineIntel" },
		{ VENDOR_AMD		, "AuthenticAMD" },
		{ VENDOR_CYRIX		, "CyrixInstead" },
		{ VENDOR_NEXGEN		, "NexGenDriven" },
		{ VENDOR_TRANSMETA	, "GenuineTMx86" },
		{ VENDOR_UMC		, "UMC UMC UMC " },
		{ VENDOR_CENTAUR	, "CentaurHauls" },
		{ VENDOR_RISE		, "RiseRiseRise" },
		{ VENDOR_SIS		, "SiS SiS SiS " },
		{ VENDOR_NSC		, "Geode by NSC" },
		{ VENDOR_HYGON		, "HygonGenuine" },
	};

	memcpy(vendor_str + 0, &raw_vendor[1], 4);
	memcpy(vendor_str + 4, &raw_vendor[3], 4);
	memcpy(vendor_str + 8, &raw_vendor[2], 4);
	vendor_str[12] = 0;

	/* Determine vendor: */
	for (i = 0; i < NUM_CPU_VENDORS; i++)
		if (!strcmp(vendor_str, matchtable[i].match)) {
			vendor = matchtable[i].vendor;
			break;
		}
	return vendor;
}

static int cpuid_basic_identify(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int i, j, basic, xmodel, xfamily, ext;
	char brandstr[64] = {0};
	data->vendor = cpuid_vendor_identify(raw->basic_cpuid[0], data->vendor_str);

	if (data->vendor == VENDOR_UNKNOWN)
		return set_error(ERR_CPU_UNKN);
	basic = raw->basic_cpuid[0][EAX];
	if (basic >= 1) {
		data->family = (raw->basic_cpuid[1][EAX] >> 8) & 0xf;
		data->model = (raw->basic_cpuid[1][EAX] >> 4) & 0xf;
		data->stepping = raw->basic_cpuid[1][EAX] & 0xf;
		xmodel = (raw->basic_cpuid[1][EAX] >> 16) & 0xf;
		xfamily = (raw->basic_cpuid[1][EAX] >> 20) & 0xff;
		if (data->vendor == VENDOR_AMD && data->family < 0xf)
			data->ext_family = data->family;
		else
			data->ext_family = data->family + xfamily;
		data->ext_model = data->model + (xmodel << 4);
	}
	ext = raw->ext_cpuid[0][EAX] - 0x8000000;

	/* obtain the brand string, if present: */
	if (ext >= 4) {
		for (i = 0; i < 3; i++)
			for (j = 0; j < 4; j++)
				memcpy(brandstr + 16ULL * i + 4ULL * j,
				       &raw->ext_cpuid[2 + i][j], 4);
		brandstr[48] = 0;
		i = 0;
		while (brandstr[i] == ' ') i++;
		strncpy(data->brand_str, brandstr + i, sizeof(data->brand_str));
		data->brand_str[48] = 0;
	}
	load_features_common(raw, data);
	data->total_logical_cpus = get_total_cpus();
	return set_error(ERR_OK);
}

/* Interface: */

int cpuid_get_total_cpus(void)
{
	return get_total_cpus();
}

int cpuid_present(void)
{
	return cpuid_exists_by_eflags();
}

void cpu_exec_cpuid(uint32_t eax, uint32_t* regs)
{
	regs[0] = eax;
	regs[1] = regs[2] = regs[3] = 0;
	exec_cpuid(regs);
}

void cpu_exec_cpuid_ext(uint32_t* regs)
{
	exec_cpuid(regs);
}

int cpuid_get_raw_data(struct cpu_raw_data_t* data)
{
	unsigned i;
	if (!cpuid_present())
		return set_error(ERR_NO_CPUID);
	for (i = 0; i < 32; i++)
		cpu_exec_cpuid(i, data->basic_cpuid[i]);
	for (i = 0; i < 32; i++)
		cpu_exec_cpuid(0x80000000 + i, data->ext_cpuid[i]);
	for (i = 0; i < MAX_INTELFN4_LEVEL; i++) {
		memset(data->intel_fn4[i], 0, sizeof(data->intel_fn4[i]));
		data->intel_fn4[i][EAX] = 4;
		data->intel_fn4[i][ECX] = i;
		cpu_exec_cpuid_ext(data->intel_fn4[i]);
	}
	for (i = 0; i < MAX_INTELFN11_LEVEL; i++) {
		memset(data->intel_fn11[i], 0, sizeof(data->intel_fn11[i]));
		data->intel_fn11[i][EAX] = 11;
		data->intel_fn11[i][ECX] = i;
		cpu_exec_cpuid_ext(data->intel_fn11[i]);
	}
	for (i = 0; i < MAX_INTELFN12H_LEVEL; i++) {
		memset(data->intel_fn12h[i], 0, sizeof(data->intel_fn12h[i]));
		data->intel_fn12h[i][EAX] = 0x12;
		data->intel_fn12h[i][ECX] = i;
		cpu_exec_cpuid_ext(data->intel_fn12h[i]);
	}
	for (i = 0; i < MAX_INTELFN14H_LEVEL; i++) {
		memset(data->intel_fn14h[i], 0, sizeof(data->intel_fn14h[i]));
		data->intel_fn14h[i][EAX] = 0x14;
		data->intel_fn14h[i][ECX] = i;
		cpu_exec_cpuid_ext(data->intel_fn14h[i]);
	}
	for (i = 0; i < MAX_AMDFN8000001DH_LEVEL; i++) {
		memset(data->amd_fn8000001dh[i], 0, sizeof(data->amd_fn8000001dh[i]));
		data->amd_fn8000001dh[i][EAX] = 0x8000001d;
		data->amd_fn8000001dh[i][ECX] = i;
		cpu_exec_cpuid_ext(data->amd_fn8000001dh[i]);
	}
	return set_error(ERR_OK);
}

int cpu_ident_internal(struct cpu_raw_data_t* raw, struct cpu_id_t* data, struct internal_id_info_t* internal)
{
	int r;
	struct cpu_raw_data_t myraw;
	if (!raw) {
		if ((r = cpuid_get_raw_data(&myraw)) < 0)
			return set_error(r);
		raw = &myraw;
	}
	cpu_id_t_constructor(data);
	if ((r = cpuid_basic_identify(raw, data)) < 0)
		return set_error(r);
	switch (data->vendor) {
		case VENDOR_INTEL:
			r = cpuid_identify_intel(raw, data, internal);
			break;
		case VENDOR_AMD:
		case VENDOR_HYGON:
			r = cpuid_identify_amd(raw, data, internal);
			break;
		default:
			break;
	}
	/* Backward compatibility */
	/* - Deprecated since v0.5.0 */
	data->l1_assoc     = data->l1_data_assoc;
	data->l1_cacheline = data->l1_data_cacheline;
	return set_error(r);
}

int cpu_identify(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	struct internal_id_info_t throwaway;
	return cpu_ident_internal(raw, data, &throwaway);
}

const char* cpu_feature_str(cpu_feature_t feature)
{
	const struct { cpu_feature_t feature; const char* name; }
	matchtable[] = {
		{ CPU_FEATURE_FPU, "fpu" },
		{ CPU_FEATURE_VME, "vme" },
		{ CPU_FEATURE_DE, "de" },
		{ CPU_FEATURE_PSE, "pse" },
		{ CPU_FEATURE_TSC, "tsc" },
		{ CPU_FEATURE_MSR, "msr" },
		{ CPU_FEATURE_PAE, "pae" },
		{ CPU_FEATURE_MCE, "mce" },
		{ CPU_FEATURE_CX8, "cx8" },
		{ CPU_FEATURE_APIC, "apic" },
		{ CPU_FEATURE_MTRR, "mtrr" },
		{ CPU_FEATURE_SEP, "sep" },
		{ CPU_FEATURE_PGE, "pge" },
		{ CPU_FEATURE_MCA, "mca" },
		{ CPU_FEATURE_CMOV, "cmov" },
		{ CPU_FEATURE_PAT, "pat" },
		{ CPU_FEATURE_PSE36, "pse36" },
		{ CPU_FEATURE_PN, "pn" },
		{ CPU_FEATURE_CLFLUSH, "clflush" },
		{ CPU_FEATURE_DTS, "dts" },
		{ CPU_FEATURE_ACPI, "acpi" },
		{ CPU_FEATURE_MMX, "mmx" },
		{ CPU_FEATURE_FXSR, "fxsr" },
		{ CPU_FEATURE_SSE, "sse" },
		{ CPU_FEATURE_SSE2, "sse2" },
		{ CPU_FEATURE_SS, "ss" },
		{ CPU_FEATURE_HT, "ht" },
		{ CPU_FEATURE_TM, "tm" },
		{ CPU_FEATURE_IA64, "ia64" },
		{ CPU_FEATURE_PBE, "pbe" },
		{ CPU_FEATURE_PNI, "pni" },
		{ CPU_FEATURE_PCLMUL, "pclmul" },
		{ CPU_FEATURE_DTS64, "dts64" },
		{ CPU_FEATURE_MONITOR, "monitor" },
		{ CPU_FEATURE_DS_CPL, "ds_cpl" },
		{ CPU_FEATURE_VMX, "vmx" },
		{ CPU_FEATURE_SMX, "smx" },
		{ CPU_FEATURE_EST, "est" },
		{ CPU_FEATURE_TM2, "tm2" },
		{ CPU_FEATURE_SSSE3, "ssse3" },
		{ CPU_FEATURE_CID, "cid" },
		{ CPU_FEATURE_CX16, "cx16" },
		{ CPU_FEATURE_XTPR, "xtpr" },
		{ CPU_FEATURE_PDCM, "pdcm" },
		{ CPU_FEATURE_DCA, "dca" },
		{ CPU_FEATURE_SSE4_1, "sse4_1" },
		{ CPU_FEATURE_SSE4_2, "sse4_2" },
		{ CPU_FEATURE_SYSCALL, "syscall" },
		{ CPU_FEATURE_XD, "xd" },
		{ CPU_FEATURE_X2APIC, "x2apic"},
		{ CPU_FEATURE_MOVBE, "movbe" },
		{ CPU_FEATURE_POPCNT, "popcnt" },
		{ CPU_FEATURE_AES, "aes" },
		{ CPU_FEATURE_XSAVE, "xsave" },
		{ CPU_FEATURE_OSXSAVE, "osxsave" },
		{ CPU_FEATURE_AVX, "avx" },
		{ CPU_FEATURE_MMXEXT, "mmxext" },
		{ CPU_FEATURE_3DNOW, "3dnow" },
		{ CPU_FEATURE_3DNOWEXT, "3dnowext" },
		{ CPU_FEATURE_NX, "nx" },
		{ CPU_FEATURE_FXSR_OPT, "fxsr_opt" },
		{ CPU_FEATURE_RDTSCP, "rdtscp" },
		{ CPU_FEATURE_LM, "lm" },
		{ CPU_FEATURE_LAHF_LM, "lahf_lm" },
		{ CPU_FEATURE_CMP_LEGACY, "cmp_legacy" },
		{ CPU_FEATURE_SVM, "svm" },
		{ CPU_FEATURE_SSE4A, "sse4a" },
		{ CPU_FEATURE_MISALIGNSSE, "misalignsse" },
		{ CPU_FEATURE_ABM, "abm" },
		{ CPU_FEATURE_3DNOWPREFETCH, "3dnowprefetch" },
		{ CPU_FEATURE_OSVW, "osvw" },
		{ CPU_FEATURE_IBS, "ibs" },
		{ CPU_FEATURE_SSE5, "sse5" },
		{ CPU_FEATURE_SKINIT, "skinit" },
		{ CPU_FEATURE_WDT, "wdt" },
		{ CPU_FEATURE_TS, "ts" },
		{ CPU_FEATURE_FID, "fid" },
		{ CPU_FEATURE_VID, "vid" },
		{ CPU_FEATURE_TTP, "ttp" },
		{ CPU_FEATURE_TM_AMD, "tm_amd" },
		{ CPU_FEATURE_STC, "stc" },
		{ CPU_FEATURE_100MHZSTEPS, "100mhzsteps" },
		{ CPU_FEATURE_HWPSTATE, "hwpstate" },
		{ CPU_FEATURE_CONSTANT_TSC, "constant_tsc" },
		{ CPU_FEATURE_XOP, "xop" },
		{ CPU_FEATURE_FMA3, "fma3" },
		{ CPU_FEATURE_FMA4, "fma4" },
		{ CPU_FEATURE_TBM, "tbm" },
		{ CPU_FEATURE_F16C, "f16c" },
		{ CPU_FEATURE_RDRAND, "rdrand" },
		{ CPU_FEATURE_CPB, "cpb" },
		{ CPU_FEATURE_APERFMPERF, "aperfmperf" },
		{ CPU_FEATURE_PFI, "pfi" },
		{ CPU_FEATURE_PA, "pa" },
		{ CPU_FEATURE_AVX2, "avx2" },
		{ CPU_FEATURE_BMI1, "bmi1" },
		{ CPU_FEATURE_BMI2, "bmi2" },
		{ CPU_FEATURE_HLE, "hle" },
		{ CPU_FEATURE_RTM, "rtm" },
		{ CPU_FEATURE_AVX512F, "avx512f" },
		{ CPU_FEATURE_AVX512DQ, "avx512dq" },
		{ CPU_FEATURE_AVX512PF, "avx512pf" },
		{ CPU_FEATURE_AVX512ER, "avx512er" },
		{ CPU_FEATURE_AVX512CD, "avx512cd" },
		{ CPU_FEATURE_SHA_NI, "sha_ni" },
		{ CPU_FEATURE_AVX512BW, "avx512bw" },
		{ CPU_FEATURE_AVX512VL, "avx512vl" },
		{ CPU_FEATURE_SGX, "sgx" },
		{ CPU_FEATURE_RDSEED, "rdseed" },
		{ CPU_FEATURE_ADX, "adx" },
		{ CPU_FEATURE_AVX512VNNI, "avx512vnni" },
		{ CPU_FEATURE_AVX512VBMI, "avx512vbmi" },
		{ CPU_FEATURE_AVX512VBMI2, "avx512vbmi2" },
	};
	unsigned i, n = COUNT_OF(matchtable);
	if (n != NUM_CPU_FEATURES) {
		//Warning: incomplete library
		//         feature matchtable size differs from the actual number of features.
	}
	for (i = 0; i < n; i++)
		if (matchtable[i].feature == feature)
			return matchtable[i].name;
	return "";
}

const char* cpuid_error(void)
{
	const struct { cpu_error_t error; const char *description; }
	matchtable[] = {
		{ ERR_OK       , "No error"},
		{ ERR_NO_CPUID , "CPUID instruction is not supported"},
		{ ERR_NO_RDTSC , "RDTSC instruction is not supported"},
		{ ERR_NO_MEM   , "Memory allocation failed"},
		{ ERR_OPEN     , "File open operation failed"},
		{ ERR_BADFMT   , "Bad file format"},
		{ ERR_NOT_IMP  , "Not implemented"},
		{ ERR_CPU_UNKN , "Unsupported processor"},
		{ ERR_NO_RDMSR , "RDMSR instruction is not supported"},
		{ ERR_NO_DRIVER, "RDMSR driver error (generic)"},
		{ ERR_NO_PERMS , "No permissions to install RDMSR driver"},
		{ ERR_EXTRACT  , "Cannot extract RDMSR driver (read only media?)"},
		{ ERR_HANDLE   , "Bad handle"},
		{ ERR_INVMSR   , "Invalid MSR"},
		{ ERR_INVCNB   , "Invalid core number"},
		{ ERR_HANDLE_R , "Error on handle read"},
		{ ERR_INVRANGE , "Invalid given range"},
	};
	unsigned i;
	for (i = 0; i < COUNT_OF(matchtable); i++)
		if (_libcpiud_errno == matchtable[i].error)
			return matchtable[i].description;
	return "Unknown error";
}


const char* cpuid_lib_version(void)
{
	return VERSION;
}

cpu_vendor_t cpuid_get_vendor(void)
{
	static cpu_vendor_t vendor = VENDOR_UNKNOWN;
	uint32_t raw_vendor[4];
	char vendor_str[VENDOR_STR_MAX];

	if(vendor == VENDOR_UNKNOWN) {
		if (!cpuid_present())
			set_error(ERR_NO_CPUID);
		else {
			cpu_exec_cpuid(0, raw_vendor);
			vendor = cpuid_vendor_identify(raw_vendor, vendor_str);
		}
	}
	return vendor;
}
