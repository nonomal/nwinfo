// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

#include <pathcch.h>

unsigned int g_init_width = 600;
unsigned int g_init_height = 800;
unsigned int g_init_alpha = 255;
nk_bool g_bginfo = 0;
unsigned int g_smart_flag = 0xFE04007E; // ~0x01FBFF81
struct nk_color g_color_warning = NK_COLOR_YELLOW;
struct nk_color g_color_error = NK_COLOR_RED;
struct nk_color g_color_good = NK_COLOR_GREEN;
struct nk_color g_color_unknown = NK_COLOR_BLUE;
struct nk_color g_color_text_l = NK_COLOR_WHITE;
struct nk_color g_color_text_d = NK_COLOR_LIGHT;
struct nk_color g_color_back = NK_COLOR_GRAY;

static LRESULT CALLBACK
window_proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_TIMER:
		gnwinfo_ctx_update(wparam);
		break;
	case WM_DPICHANGED:
		break;
	case WM_SIZE:
		g_ctx.gui_height = HIWORD(lparam);
		g_ctx.gui_width = LOWORD(lparam);
		break;
	}
	if (nk_gdip_handle_event(wnd, msg, wparam, lparam))
		return 0;
	return DefWindowProcW(wnd, msg, wparam, lparam);
}

static void
get_ini_color(LPCWSTR key, struct nk_color* color)
{
	WCHAR fallback[7];
	UINT32 hex;
	swprintf(fallback, 7, L"%02X%02X%02X", color->r, color->g, color->b);
	hex = strtoul(gnwinfo_get_ini_value(L"Color", key, fallback), NULL, 16);
	color->r = (hex >> 16) & 0xFF;
	color->g = (hex >> 8) & 0xFF;
	color->b = hex & 0xFF;
}

static struct nk_color
convert_color(struct nk_color color, float offset)
{
	float h, s, v;
	nk_color_hsv_f(&h, &s, &v, color);
	v += offset;
	return nk_hsv_f(h, s, v);
}

static void
set_style(struct nk_context* ctx)
{
	struct nk_color text_p10 = convert_color(g_color_text_d, -0.10f);
	struct nk_color text_p20 = convert_color(g_color_text_d, -0.20f);
	struct nk_color text_p30 = convert_color(g_color_text_d, -0.30f);
	struct nk_color back_p2 = convert_color(g_color_back, -0.02f);
	struct nk_color back_p3 = convert_color(g_color_back, -0.03f);
	struct nk_color back_p4 = convert_color(g_color_back, -0.04f);
	struct nk_color back_n2 = convert_color(g_color_back, 0.02f);

	ctx->style.text.color = g_color_text_d;

	ctx->style.window.background = g_color_back;
	ctx->style.window.fixed_background = nk_style_item_color(g_color_back);
	ctx->style.window.min_row_height_padding = 2;
	ctx->style.window.header.normal = nk_style_item_color(back_p2);
	ctx->style.window.header.hover = nk_style_item_color(back_p2);
	ctx->style.window.header.active = nk_style_item_color(back_p2);
	ctx->style.window.header.label_normal = g_color_text_d;
	ctx->style.window.header.label_hover = g_color_text_d;
	ctx->style.window.header.label_active = g_color_text_d;
	ctx->style.window.header.close_button.normal = nk_style_item_color(back_p2);
	ctx->style.window.header.close_button.hover = nk_style_item_color(back_p2);
	ctx->style.window.header.close_button.active = nk_style_item_color(back_p2);
	ctx->style.window.header.close_button.text_normal = g_color_text_d;
	ctx->style.window.header.close_button.text_hover = g_color_text_d;
	ctx->style.window.header.close_button.text_active = g_color_text_d;

	ctx->style.button.text_normal = g_color_text_d;
	ctx->style.button.text_hover = g_color_text_d;
	ctx->style.button.text_active = g_color_text_d;
	ctx->style.button.normal = nk_style_item_color(back_n2);
	ctx->style.button.hover = nk_style_item_color(back_p2);
	ctx->style.button.active = nk_style_item_color(back_p4);
	ctx->style.button.rounding = 0;
	ctx->style.button.border = 1.0f;
	ctx->style.button.padding = nk_vec2(0.0f, 0.0f);

	ctx->style.checkbox.text_normal = g_color_text_d;
	ctx->style.checkbox.text_hover = g_color_text_d;
	ctx->style.checkbox.text_active = g_color_text_d;
	ctx->style.checkbox.cursor_normal = nk_style_item_color(g_color_back);
	ctx->style.checkbox.cursor_hover = nk_style_item_color(g_color_back);
	ctx->style.checkbox.normal = nk_style_item_color(text_p30);
	ctx->style.checkbox.hover = nk_style_item_color(text_p20);
	ctx->style.checkbox.active = nk_style_item_color(text_p20);

	ctx->style.slider.bar_normal = back_p3;
	ctx->style.slider.bar_hover = back_p3;
	ctx->style.slider.bar_active = back_p3;
	ctx->style.slider.bar_filled = text_p30;
	ctx->style.slider.cursor_normal = nk_style_item_color(text_p30);
	ctx->style.slider.cursor_hover = nk_style_item_color(text_p20);
	ctx->style.slider.cursor_active = nk_style_item_color(text_p10);

	ctx->style.progress.normal = nk_style_item_color(back_p3);
	ctx->style.progress.hover = nk_style_item_color(back_p3);
	ctx->style.progress.active = nk_style_item_color(back_p3);

	ctx->style.combo.normal = nk_style_item_color(g_color_back);
	ctx->style.combo.hover = nk_style_item_color(g_color_back);
	ctx->style.combo.active = nk_style_item_color(g_color_back);
	ctx->style.combo.button.normal = nk_style_item_color(g_color_back);
	ctx->style.combo.button.hover = nk_style_item_color(g_color_back);
	ctx->style.combo.button.active = nk_style_item_color(g_color_back);
	ctx->style.combo.button.text_normal = g_color_text_d;
	ctx->style.combo.button.text_hover = g_color_text_d;
	ctx->style.combo.button.text_active = g_color_text_d;

	ctx->style.property.normal = nk_style_item_color(back_p3);
	ctx->style.property.hover = nk_style_item_color(back_p3);
	ctx->style.property.active = nk_style_item_color(back_p3);
	ctx->style.property.label_normal = g_color_text_d;
	ctx->style.property.label_hover = g_color_text_d;
	ctx->style.property.label_active = g_color_text_d;
	ctx->style.property.dec_button.normal = nk_style_item_color(back_p3);
	ctx->style.property.dec_button.hover = nk_style_item_color(back_p3);
	ctx->style.property.dec_button.active = nk_style_item_color(back_p3);
	ctx->style.property.dec_button.text_normal = g_color_text_d;
	ctx->style.property.dec_button.text_hover = g_color_text_d;
	ctx->style.property.dec_button.text_active = g_color_text_d;
	ctx->style.property.inc_button = ctx->style.property.dec_button;
	ctx->style.property.edit.normal = nk_style_item_color(back_p3);
	ctx->style.property.edit.hover = nk_style_item_color(back_p3);
	ctx->style.property.edit.active = nk_style_item_color(back_p3);
	ctx->style.property.edit.cursor_normal = g_color_text_d;
	ctx->style.property.edit.cursor_hover = g_color_text_d;
	ctx->style.property.edit.cursor_text_normal = back_p3;
	ctx->style.property.edit.cursor_text_hover = back_p3;
	ctx->style.property.edit.text_normal = g_color_text_d;
	ctx->style.property.edit.text_hover = g_color_text_d;
	ctx->style.property.edit.text_active = g_color_text_d;
	ctx->style.property.edit.selected_normal = g_color_text_d;
	ctx->style.property.edit.selected_hover = g_color_text_d;
	ctx->style.property.edit.selected_text_normal = back_p3;
	ctx->style.property.edit.selected_text_hover = back_p3;

	ctx->style.scrollh.normal = nk_style_item_color(back_p2);
	ctx->style.scrollh.hover = nk_style_item_color(back_p2);
	ctx->style.scrollh.active = nk_style_item_color(back_p2);
	ctx->style.scrollh.cursor_normal = nk_style_item_color(text_p30);
	ctx->style.scrollh.cursor_hover = nk_style_item_color(text_p20);
	ctx->style.scrollh.cursor_active = nk_style_item_color(text_p10);
	ctx->style.scrollv = ctx->style.scrollh;
}

int APIENTRY
wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	GdipFont* font;
	struct nk_context* ctx;
	const char* str;
	int x_pos = 100, y_pos = 100;
	WNDCLASSW wc;
	RECT rect;
	DWORD style = WS_SIZEBOX | WS_VISIBLE;
	DWORD exstyle = WS_EX_LAYERED;
	HWND wnd;
	int running = 1;
	int needs_refresh = 1;
	WCHAR font_name[64];

	GetModuleFileNameW(NULL, g_ini_path, MAX_PATH);
	PathCchRemoveFileSpec(g_ini_path, MAX_PATH);
	PathCchAppend(g_ini_path, MAX_PATH, L"gnwinfo.ini");

	g_smart_flag = strtoul(gnwinfo_get_ini_value(L"Widgets", L"SmartFlags", L"0xFE04007E"), NULL, 16);
	g_init_width = strtoul(gnwinfo_get_ini_value(L"Window", L"Width", L"600"), NULL, 10);
	g_init_height = strtoul(gnwinfo_get_ini_value(L"Window", L"Height", L"800"), NULL, 10);
	g_init_alpha = strtoul(gnwinfo_get_ini_value(L"Window", L"Alpha", L"255"), NULL, 10);
	str = gnwinfo_get_ini_value(L"Window", L"BGInfo", L"0");
	if (str[0] != '0')
	{
		RECT desktop = {0, 0, 1024, 768};
		g_bginfo = 1;
		exstyle |= WS_EX_NOACTIVATE;
		GetWindowRect(GetDesktopWindow(), &desktop);
		x_pos = desktop.right > (LONG)g_init_width ? (desktop.right - (LONG)g_init_width) : 0;
		y_pos = 0;
	}
	get_ini_color(L"Background", &g_color_back);
	get_ini_color(L"Highlight", &g_color_text_l);
	get_ini_color(L"Default", &g_color_text_d);
	get_ini_color(L"StateGood", &g_color_good);
	get_ini_color(L"StateWarn", &g_color_warning);
	get_ini_color(L"StateError", &g_color_error);
	get_ini_color(L"StateUnknown", &g_color_unknown);
	rect.left = x_pos;
	rect.right = x_pos + g_init_width;
	rect.top = y_pos;
	rect.bottom = y_pos + g_init_height;
	if (!g_bginfo)
		AdjustWindowRectExForDpi(&rect, style, FALSE, exstyle, USER_DEFAULT_SCREEN_DPI);

	/* Win32 */
	memset(&wc, 0, sizeof(wc));
	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = window_proc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON1));
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wc.lpszClassName = L"NwinfoWindowClass";
	RegisterClassW(&wc);

	wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"NWinfo GUI",
		style, rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top,
		NULL, NULL, wc.hInstance, NULL);

	if (g_bginfo)
	{
		style = (DWORD)GetWindowLongPtrW(wnd, GWL_STYLE);
		SetWindowLongPtrW(wnd, GWL_STYLE, style & ~WS_CAPTION & ~WS_SIZEBOX);
		SetWindowPos(wnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
	SetLayeredWindowAttributes(wnd, 0, (BYTE)g_init_alpha, LWA_ALPHA);

	/* GUI */
	ctx = nk_gdip_init(wnd, g_init_width, g_init_height);
	GetPrivateProfileStringW(L"Window", L"Font", L"Courier New", font_name, 64, g_ini_path);
	font = nk_gdip_load_font(font_name, GNWINFO_FONT_SIZE, IDR_FONT1);
	nk_gdip_set_font(font);

	(void)CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	set_style(ctx);
	gnwinfo_ctx_init(hInstance, wnd, ctx, (float)g_init_width, (float)g_init_height);

	g_ctx.main_flag = strtoul(gnwinfo_get_ini_value(L"Widgets", L"HideComponents", L"0xFFFFFFFF"), NULL, 16);
	g_ctx.smart_hex = strtoul(gnwinfo_get_ini_value(L"Widgets", L"SmartFormat", L"0"), NULL, 10);

	while (running)
	{
		/* Input */
		MSG msg;
		nk_input_begin(ctx);
		if (needs_refresh == 0)
		{
			if (GetMessageW(&msg, NULL, 0, 0) <= 0)
				running = 0;
			else
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
			needs_refresh = 1;
		}
		else
			needs_refresh = 0;
		while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				running = 0;
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
			needs_refresh = 1;
		}
		nk_input_end(ctx);

		/* GUI */
		if (g_ctx.gui_settings == TRUE)
			set_style(ctx);
		gnwinfo_draw_main_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_cpuid_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_about_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_smart_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_settings_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_pci_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_dmi_window(ctx, g_ctx.gui_width, g_ctx.gui_height);

		/* Draw */
		nk_gdip_render(NK_ANTI_ALIASING_ON, (struct nk_color)NK_COLOR_BLACK);
	}

	CoUninitialize();
	nk_gdipfont_del(font);
	nk_gdip_shutdown();
	UnregisterClassW(wc.lpszClassName, wc.hInstance);
	gnwinfo_ctx_exit();
	return 0;
}
