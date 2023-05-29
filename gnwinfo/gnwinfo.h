// SPDX-License-Identifier: Unlicense

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <pdh.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR

#include <assert.h>
#define NK_ASSERT(expr) assert(expr)

#include <nuklear.h>
#include <nuklear_gdip.h>

#include <libnw.h>

#include "resource.h"

#define GNWINFO_FONT_SIZE 12

#define MAIN_INFO_OS        (1U << 0)
#define MAIN_INFO_BIOS      (1U << 1)
#define MAIN_INFO_BOARD     (1U << 2)
#define MAIN_INFO_CPU       (1U << 3)
#define MAIN_INFO_MEMORY    (1U << 4)
#define MAIN_INFO_MONITOR   (1U << 5)
#define MAIN_INFO_STORAGE   (1U << 6)
#define MAIN_INFO_NETWORK   (1U << 7)

#define MAIN_NET_ACTIVE     (1U << 16)

typedef struct _GNW_CONTEXT
{
	HINSTANCE inst;
	HWND wnd;
	HANDLE mutex;
	NWLIB_CONTEXT lib;
	struct nk_context* nk;
	float gui_width;
	float gui_height;
	UINT32 main_flag;
	BOOL gui_cpuid;
	BOOL gui_smart;
	BOOL gui_about;
	BOOL gui_settings;
	nk_bool smart_hex;

	struct nk_image image_os;
	struct nk_image image_bios;
	struct nk_image image_board;
	struct nk_image image_cpu;
	struct nk_image image_ram;
	struct nk_image image_edid;
	struct nk_image image_disk;
	struct nk_image image_net;
	struct nk_image image_close;
	struct nk_image image_dir;
	struct nk_image image_info;
	struct nk_image image_refresh;
	struct nk_image image_set;

	PNODE system;
	PNODE cpuid;
	PNODE smbios;
	PNODE network;
	PNODE disk;
	PNODE edid;
	PNODE pci;
	PNODE uefi;

	PDH_HQUERY pdh;
	PDH_HCOUNTER pdh_cpu;
	PDH_HCOUNTER pdh_net_recv;
	PDH_HCOUNTER pdh_net_send;
	double pdh_val_cpu;
	CHAR net_recv[48];
	CHAR net_send[48];

	LPCSTR sys_uptime;
	MEMORYSTATUSEX mem_status;
	CHAR mem_total[48];
	CHAR mem_avail[48];
} GNW_CONTEXT;
extern GNW_CONTEXT g_ctx;

#define NK_COLOR_YELLOW     nk_rgb(0xFF, 0xEA, 0x00)
#define NK_COLOR_RED        nk_rgb(0xFF, 0x17, 0x44)
#define NK_COLOR_GREEN      nk_rgb(0x00, 0xE6, 0x76)
#define NK_COLOR_CYAN       nk_rgb(0x03, 0xDA, 0xC6)
#define NK_COLOR_BLUE       nk_rgb(0x29, 0x79, 0xFF)
#define NK_COLOR_WHITE      nk_rgb(0xFF, 0xFF, 0xFF)
#define NK_COLOR_BLACK      nk_rgb(0x00, 0x00, 0x00)
#define NK_COLOR_GRAY       nk_rgb(0x1E, 0x1E, 0x1E)

void gnwinfo_ctx_update(WPARAM wparam);
void gnwinfo_ctx_init(HINSTANCE inst, HWND wnd, struct nk_context* ctx, float width, float height);
void gnwinfo_ctx_exit();
VOID gnwinfo_draw_main_window(struct nk_context* ctx, float width, float height);
VOID gnwinfo_draw_cpuid_window(struct nk_context* ctx, float width, float height);
VOID gnwinfo_draw_about_window(struct nk_context* ctx, float width, float height);
VOID gnwinfo_draw_smart_window(struct nk_context* ctx, float width, float height);
VOID gnwinfo_draw_settings_window(struct nk_context* ctx, float width, float height);

LPCSTR gnwinfo_get_node_attr(PNODE node, LPCSTR key);
