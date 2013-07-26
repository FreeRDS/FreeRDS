/**
 * xrdp: A Remote Desktop Protocol server.
 * Graphics Pipeline
 *
 * Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xrdp.h"

int xrdp_message_server_begin_update(xrdpModule* mod)
{
	int status;
	status = mod->ServerProxy->BeginUpdate(mod);
	return status;
}

int xrdp_message_server_end_update(xrdpModule* mod)
{
	int status;
	status = mod->ServerProxy->EndUpdate(mod);
	return status;
}

int xrdp_message_server_beep(xrdpModule* mod)
{
	int status;
	status = mod->ServerProxy->Beep(mod);
	return status;
}

int xrdp_message_server_message(xrdpModule* mod, char* msg, int code)
{
	int status;
	status = mod->ServerProxy->Message(mod, msg, code);
	return status;
}

int xrdp_message_server_is_terminated(xrdpModule* mod)
{
	int status;
	status = mod->ServerProxy->IsTerminated(mod);
	return status;
}

int xrdp_message_server_opaque_rect(xrdpModule* mod, XRDP_MSG_OPAQUE_RECT* msg)
{
	int status;
	status = mod->ServerProxy->OpaqueRect(mod, msg);
	return status;
}

int xrdp_message_server_screen_blt(xrdpModule* mod, XRDP_MSG_SCREEN_BLT* msg)
{
	int status;
	status = mod->ServerProxy->ScreenBlt(mod, msg);
	return status;
}

int xrdp_message_server_paint_rect(xrdpModule* mod, XRDP_MSG_PAINT_RECT* msg)
{
	int status;
	status = mod->ServerProxy->PaintRect(mod, msg);
	return status;
}

int xrdp_message_server_patblt(xrdpModule* mod, XRDP_MSG_PATBLT* msg)
{
	int status;
	status = mod->ServerProxy->PatBlt(mod, msg);
	return status;
}

int xrdp_message_server_set_pointer(xrdpModule* mod, XRDP_MSG_SET_POINTER* msg)
{
	int status;
	status = mod->ServerProxy->SetPointer(mod, msg);
	return status;
}

int xrdp_message_server_set_palette(xrdpModule* mod, int* palette)
{
	int status;
	status = mod->ServerProxy->SetPalette(mod, palette);
	return status;
}

int xrdp_message_server_set_clipping_region(xrdpModule* mod, XRDP_MSG_SET_CLIPPING_REGION* msg)
{
	int status;
	status = mod->ServerProxy->SetClippingRegion(mod, msg);
	return status;
}

int xrdp_message_server_set_null_clipping_region(xrdpModule* mod)
{
	int status;
	status = mod->ServerProxy->SetNullClippingRegion(mod);
	return status;
}

int xrdp_message_server_set_forecolor(xrdpModule* mod, int fgcolor)
{
	int status;
	status = mod->ServerProxy->SetForeColor(mod, fgcolor);
	return status;
}

int xrdp_message_server_set_backcolor(xrdpModule* mod, int bgcolor)
{
	int status;
	status = mod->ServerProxy->SetBackColor(mod, bgcolor);
	return status;
}

int xrdp_message_server_set_rop2(xrdpModule* mod, int opcode)
{
	int status;
	status = mod->ServerProxy->SetRop2(mod, opcode);
	return status;
}

int xrdp_message_server_line_to(xrdpModule* mod, XRDP_MSG_LINE_TO* msg)
{
	int status;
	status = mod->ServerProxy->LineTo(mod, msg);
	return status;
}

int xrdp_message_server_add_char(xrdpModule* mod, XRDP_MSG_CACHE_GLYPH* msg)
{
	int status;
	status = mod->ServerProxy->AddChar(mod, msg);
	return status;
}

int xrdp_message_server_text(xrdpModule* mod, GLYPH_INDEX_ORDER* msg)
{
	int status;
	status = mod->ServerProxy->Text(mod, msg);
	return status;
}

int xrdp_message_server_reset(xrdpModule* mod, int width, int height, int bpp)
{
	int status;
	status = mod->ServerProxy->Reset(mod, width, height, bpp);
	return status;
}

int xrdp_message_server_create_offscreen_surface(xrdpModule* mod, int rdpindex, int width, int height)
{
	int status;
	status = mod->ServerProxy->CreateOffscreenSurface(mod, rdpindex, width, height);
	return status;
}

int xrdp_message_server_switch_offscreen_surface(xrdpModule* mod, int rdpindex)
{
	int status;
	status = mod->ServerProxy->SwitchOffscreenSurface(mod, rdpindex);
	return status;
}

int xrdp_message_server_delete_offscreen_surface(xrdpModule* mod, int rdpindex)
{
	int status;
	status = mod->ServerProxy->DeleteOffscreenSurface(mod, rdpindex);
	return status;
}

int xrdp_message_server_paint_offscreen_rect(xrdpModule* mod, int x, int y, int cx, int cy, int rdpindex, int srcx, int srcy)
{
	int status;
	status = mod->ServerProxy->PaintOffscreenRect(mod, x, y, cx, cy, rdpindex, srcx, srcy);
	return status;
}

int xrdp_message_server_window_new_update(xrdpModule* mod, XRDP_MSG_WINDOW_NEW_UPDATE* msg)
{
	int status;
	status = mod->ServerProxy->WindowNewUpdate(mod, msg);
	return status;
}

int xrdp_message_server_window_delete(xrdpModule* mod, XRDP_MSG_WINDOW_DELETE* msg)
{
	int status;
	status = mod->ServerProxy->WindowDelete(mod, msg);
	return status;
}

int xrdp_message_server_window_icon(xrdpModule* mod, int window_id, int cache_entry, int cache_id,
		xrdpRailIconInfo* icon_info, int flags)
{
	int status;
	status = mod->ServerProxy->WindowIcon(mod, window_id, cache_entry, cache_id, icon_info, flags);
	return status;
}

int xrdp_message_server_window_cached_icon(xrdpModule* mod, int window_id, int cache_entry, int cache_id, int flags)
{
	int status;
	status = mod->ServerProxy->WindowCachedIcon(mod, window_id, cache_entry, cache_id, flags);
	return status;
}

int xrdp_message_server_notify_new_update(xrdpModule* mod, int window_id, int notify_id,
		xrdpRailNotifyStateOrder* notify_state, int flags)
{
	int status;
	status = mod->ServerProxy->NotifyNewUpdate(mod, window_id, notify_id, notify_state, flags);
	return status;
}

int xrdp_message_server_notify_delete(xrdpModule* mod, int window_id, int notify_id)
{
	int status;
	status = mod->ServerProxy->NotifyDelete(mod, window_id, notify_id);
	return status;
}

int xrdp_message_server_monitored_desktop(xrdpModule* mod, xrdpRailMonitoredDesktopOrder* mdo, int flags)
{
	int status;
	status = mod->ServerProxy->MonitoredDesktop(mod, mdo, flags);
	return status;
}

int xrdp_message_server_module_init(xrdpModule* mod)
{
	mod->ServerProxy = (xrdpServerModule*) malloc(sizeof(xrdpServerModule));

	if (mod->server)
	{
		CopyMemory(mod->ServerProxy, mod->server, sizeof(xrdpServerModule));

		mod->server->BeginUpdate = xrdp_message_server_begin_update;
		mod->server->EndUpdate = xrdp_message_server_end_update;
		mod->server->Beep = xrdp_message_server_beep;
		mod->server->Message = xrdp_message_server_message;
		mod->server->IsTerminated = xrdp_message_server_is_terminated;
		mod->server->OpaqueRect = xrdp_message_server_opaque_rect;
		mod->server->ScreenBlt = xrdp_message_server_screen_blt;
		mod->server->PaintRect = xrdp_message_server_paint_rect;
		mod->server->PatBlt = xrdp_message_server_patblt;
		mod->server->SetPointer = xrdp_message_server_set_pointer;
		mod->server->SetPalette = xrdp_message_server_set_palette;
		mod->server->SetClippingRegion = xrdp_message_server_set_clipping_region;
		mod->server->SetNullClippingRegion = xrdp_message_server_set_null_clipping_region;
		mod->server->SetForeColor = xrdp_message_server_set_forecolor;
		mod->server->SetBackColor = xrdp_message_server_set_backcolor;
		mod->server->SetRop2 = xrdp_message_server_set_rop2;
		mod->server->LineTo = xrdp_message_server_line_to;
		mod->server->AddChar = xrdp_message_server_add_char;
		mod->server->Text = xrdp_message_server_text;
		mod->server->Reset = xrdp_message_server_reset;
		mod->server->CreateOffscreenSurface = xrdp_message_server_create_offscreen_surface;
		mod->server->SwitchOffscreenSurface = xrdp_message_server_switch_offscreen_surface;
		mod->server->DeleteOffscreenSurface = xrdp_message_server_delete_offscreen_surface;
		mod->server->PaintOffscreenRect = xrdp_message_server_paint_offscreen_rect;
		mod->server->WindowNewUpdate = xrdp_message_server_window_new_update;
		mod->server->WindowDelete = xrdp_message_server_window_delete;
		mod->server->WindowIcon = xrdp_message_server_window_icon;
		mod->server->WindowCachedIcon = xrdp_message_server_window_cached_icon;
		mod->server->NotifyNewUpdate = xrdp_message_server_notify_new_update;
		mod->server->NotifyDelete = xrdp_message_server_notify_delete;
		mod->server->MonitoredDesktop = xrdp_message_server_monitored_desktop;
	}

	return 0;
}
