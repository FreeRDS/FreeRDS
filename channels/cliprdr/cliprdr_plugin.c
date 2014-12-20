/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * FreeRDS channel server plugin for CLIPRDR
 *
 * Copyright 2014 Dell Software <Mike.McDonald@software.dell.com>
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

/**
 * Inter-Client Communication Conventions Manual:
 * http://www.x.org/docs/ICCCM/icccm.pdf
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <winpr/wlog.h>
#include <winpr/wtypes.h>
#include <winpr/clipboard.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#ifdef WITH_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

#include <freerdp/server/cliprdr.h>

#include "channel_plugin.h"

struct _CLIPRDR_PLUGIN_CONTEXT
{
	wLog* log;

	BOOL sync;
	HANDLE Thread;
	HANDLE StopEvent;
	HANDLE X11Event;
	HANDLE ChannelEvent;
	wClipboard* system;
	CliprdrServerContext* cliprdr;

	UINT32 requestedFormatId;
	Atom requestedFormatAtom;

	UINT32 serverRequestedFormatId;
	Atom serverRequestedFormatAtom;

	UINT32 numTargets;
	char** targets;

	UINT32 numClientFormats;
	CLIPRDR_FORMAT* clientFormats;

	XEvent* response;

	GC gc;
	int xfds;
	int depth;
	Display* display;
	Screen* screen;
	Visual* visual;
	Window owner;
	Window window;
	Window root_window;
	int screen_number;
	unsigned long border;
	unsigned long background;

	Atom incr_atom;
	Atom targets_atom;
	Atom timestamp_atom;
	Atom clipboard_atom;
	Atom property_atom;
	Atom identity_atom;

	int xfixes_event_base;
	int xfixes_error_base;
	BOOL xfixes_supported;
};
typedef struct _CLIPRDR_PLUGIN_CONTEXT CLIPRDR_PLUGIN_CONTEXT;

/**
 * plain text in gedit:
 *
 * TIMESTAMP
 * TARGETS
 * MULTIPLE
 * SAVE_TARGETS
 * GTK_TEXT_BUFFER_CONTENTS
 * application/x-gtk-text-buffer-rich-text
 * UTF8_STRING
 * COMPOUND_TEXT
 * TEXT
 * STRING
 * text/plain;charset=utf-8
 * text/plain
 */

/**
 * rich text in LibreOffice Writer:
 *
 * text/plain;charset=utf-8
 * text/plain;charset=UTF-8
 * UTF-8
 * UTF8_STRING
 * COMPOUND_TEXT
 * STRING
 * text/richtext
 * text/html
 * MULTIPLE
 */

/**
 * html content in firefox:
 *
 * TIMESTAMP
 * TARGETS
 * MULTIPLE
 * SAVE_TARGETS
 * text/html
 * text/_moz_htmlcontext
 * text/_moz_htmlinfo
 * UTF8_STRING
 * COMPOUND_TEXT
 * TEXT
 * STRING
 * text/x-moz-url-priv
 */

/**
 * file in nautilus:
 *
 * text/uri-list
 * text/x-moz-url
 * text/plain
 * UTF8_STRING
 * STRING
 * TEXT
 * COMPOUND_TEXT
 * TARGETS
 * MULTIPLE
 * TIMESTAMP
 * SAVE_TARGETS
 */

/**
 * image in kolourpaint:
 *
 * application/x-kolourpaint-selection-400
 * application/x-qt-image
 * image/png
 * image/bw
 * image/eps
 * image/epsf
 * image/epsi
 * image/pcx
 * image/rgb
 * image/rgba
 * image/sgi
 * image/tga
 * image/bmp
 * image/ico
 * image/jp2
 * image/jpeg
 * image/jpg
 * image/pic
 * image/ppm
 * PIXMAP
 * image/tif
 * image/tiff
 * image/webp
 * image/xbm
 * image/xpm
 * image/xv
 * TARGETS
 * MULTIPLE
 * TIMESTAMP
 * SAVE_TARGETS
 */

char** freerds_cliprdr_get_target_list(CLIPRDR_PLUGIN_CONTEXT* context, UINT32* count)
{
	int index;
	Atom atom;
	char* target;
	char** targets;
	Atom* atoms = NULL;
	int format_property;
	unsigned long length;
	unsigned long bytes_left;

	*count = 0;

	XGetWindowProperty(context->display, context->window, context->property_atom,
		0, 200, 0, XA_ATOM, &atom, &format_property, &length, &bytes_left, (BYTE**) &atoms);

	targets = (char**) malloc(sizeof(char*) * length);

	if (!targets)
		return NULL;

	*count = length;

	for (index = 0; index < length; index++)
	{
		target = XGetAtomName(context->display, atoms[index]);
		targets[index] = _strdup(target);
		XFree(target);
	}

	XFree(atoms);

	return targets;
}

Atom* freerds_cliprdr_get_target_atom_list(CLIPRDR_PLUGIN_CONTEXT* context, char** targets, UINT32 count)
{
	Atom* atoms;
	UINT32 index;

	atoms = (Atom*) calloc(count, sizeof(Atom));

	if (!atoms)
		return NULL;

	for (index = 0; index < count; index++)
	{
		atoms[index] = XInternAtom(context->display, targets[index], False);
	}

	return atoms;
}

BOOL freerds_cliprdr_is_target_in_list(char** targets, UINT32 count, const char* name)
{
	UINT32 index;
	BOOL found = FALSE;

	for (index = 0; index < count; index++)
	{
		if (strcmp(targets[index], name) == 0)
		{
			found = TRUE;
			break;
		}
	}

	return found;
}

BOOL freerds_cliprdr_add_target_to_list(char** targets, UINT32* count, const char* name)
{
	BOOL found;

	found = freerds_cliprdr_is_target_in_list(targets, *count, name);

	if (!found)
	{
		targets[*count] = _strdup(name);
		(*count)++;
	}

	return found;
}

int freerds_cliprdr_build_target_list(CLIPRDR_PLUGIN_CONTEXT* context)
{
	UINT32 index;
	char** targets;
	UINT32 numTargets;
	UINT32 numTargetsMax;
	CLIPRDR_FORMAT* format;

	numTargets = 0;
	numTargetsMax = context->numClientFormats + 16;

	targets = (char**) calloc(numTargetsMax, sizeof(char*));

	freerds_cliprdr_add_target_to_list(targets, &numTargets, "TIMESTAMP");
	freerds_cliprdr_add_target_to_list(targets, &numTargets, "TARGETS");

	for (index = 0; index < context->numClientFormats; index++)
	{
		format = &(context->clientFormats[index]);

		if (format->formatId < CF_MAX)
		{
			switch (format->formatId)
			{
				case CF_TEXT:
				case CF_OEMTEXT:
				case CF_UNICODETEXT:
					freerds_cliprdr_add_target_to_list(targets, &numTargets, "UTF8_STRING");
					break;

				case CF_DIB:
					freerds_cliprdr_add_target_to_list(targets, &numTargets, "image/bmp");
					break;
			}
		}
		else
		{
			if (strcmp(format->formatName, "HTML Format") == 0)
			{
				freerds_cliprdr_add_target_to_list(targets, &numTargets, "text/html");
			}
		}
	}

	context->targets = targets;
	context->numTargets = numTargets;

	return 1;
}

BOOL freerds_cliprdr_is_format_in_list(CLIPRDR_FORMAT* formats, UINT32 numFormats, UINT32 formatId, const char* name)
{
	UINT32 index;
	CLIPRDR_FORMAT* format = NULL;

	if (formatId)
	{
		for (index = 0; index < numFormats; index++)
		{
			if (formatId == formats[index].formatId)
			{
				format = &formats[index];
				break;
			}
		}
	}
	else if (name)
	{
		for (index = 0; index < numFormats; index++)
		{
			if (!formats[index].formatName)
				continue;

			if (strcmp(name, formats[index].formatName) == 0)
			{
				format = &formats[index];
				break;
			}
		}
	}

	return format ? TRUE : FALSE;
}

UINT32 freerds_cliprdr_find_matching_target_format_id(CLIPRDR_PLUGIN_CONTEXT* context, const char* target)
{
	UINT32 formatId = 0;
	UINT32 numFormats = context->numClientFormats;
	CLIPRDR_FORMAT* formats = context->clientFormats;

	if (!freerds_cliprdr_is_target_in_list(context->targets, context->numTargets, target))
		return 0;

	if (freerds_cliprdr_is_format_in_list(formats, numFormats, 0, target))
	{
		formatId = ClipboardGetFormatId(context->system, target);

		if (formatId)
			return formatId;
	}

	if (strcmp(target, "text/html") == 0)
	{
		formatId = ClipboardGetFormatId(context->system, "HTML Format");
	}
	else if (strcmp(target, "image/bmp") == 0)
	{
		formatId = CF_DIB;
	}
	else if (strcmp(target, "UTF8_STRING") == 0)
	{
		formatId = CF_UNICODETEXT;
	}

	return formatId;
}

void freerds_cliprdr_free_target_list(char** targets, int count)
{
	int index;

	for (index = 0; index < count; index++)
	{
		free(targets[index]);
	}

	free(targets);
}

void freerds_cliprdr_free_format_list(CLIPRDR_FORMAT* formats, UINT32 numFormats)
{
	UINT32 index;

	for (index = 0; index < numFormats; index++)
	{
		free(formats[index].formatName);
	}

	free(formats);
}

static int freerds_cliprdr_client_capabilities(CliprdrServerContext* cliprdr, CLIPRDR_CAPABILITIES* capabilities)
{
	CLIPRDR_PLUGIN_CONTEXT* context = (CLIPRDR_PLUGIN_CONTEXT*) cliprdr->custom;
	context->sync = TRUE;

	fprintf(stderr, "ClientCapabilities\n");

	return 1;
}

static int freerds_cliprdr_client_format_list(CliprdrServerContext* cliprdr, CLIPRDR_FORMAT_LIST* formatList)
{
	UINT32 index;
	CLIPRDR_FORMAT_LIST_RESPONSE formatListResponse;
	CLIPRDR_PLUGIN_CONTEXT* context = (CLIPRDR_PLUGIN_CONTEXT*) cliprdr->custom;

	fprintf(stderr, "ClientFormatList\n");

	freerds_cliprdr_free_format_list(context->clientFormats, context->numClientFormats);
	context->numClientFormats = formatList->numFormats;
	context->clientFormats = NULL;

	if (formatList->numFormats)
	{
		context->clientFormats = (CLIPRDR_FORMAT*) calloc(formatList->numFormats, sizeof(CLIPRDR_FORMAT));

		if (!context->clientFormats)
			return -1;
	}

	for (index = 0; index < formatList->numFormats; index++)
	{
		context->clientFormats[index].formatId = formatList->formats[index].formatId;
		context->clientFormats[index].formatName = _strdup(formatList->formats[index].formatName);

		fprintf(stderr, "[%d]\tFormat Id: 0x%04X Name: %s\n",
				index, formatList->formats[index].formatId, formatList->formats[index].formatName);
	}

	formatListResponse.msgType = CB_FORMAT_LIST_RESPONSE;
	formatListResponse.msgFlags = CB_RESPONSE_OK;
	formatListResponse.dataLen = 0;

	cliprdr->ServerFormatListResponse(cliprdr, &formatListResponse);

	freerds_cliprdr_build_target_list(context);

	XSetSelectionOwner(context->display, context->clipboard_atom, context->window, CurrentTime);

	return 1;
}

static int freerds_cliprdr_client_format_list_response(CliprdrServerContext* cliprdr, CLIPRDR_FORMAT_LIST_RESPONSE* formatListResponse)
{
	fprintf(stderr, "ClientFormatListResponse: 0x%04X\n", formatListResponse->msgFlags);
	return 1;
}

static int freerds_cliprdr_client_format_data_request(CliprdrServerContext* cliprdr, CLIPRDR_FORMAT_DATA_REQUEST* formatDataRequest)
{
	char* target = NULL;
	UINT32 formatId = 0;
	const char* name = NULL;
	CLIPRDR_PLUGIN_CONTEXT* context = (CLIPRDR_PLUGIN_CONTEXT*) cliprdr->custom;

	formatId = formatDataRequest->requestedFormatId;
	name = ClipboardGetFormatName(context->system, formatId);

	fprintf(stderr, "ClientFormatDataRequest: 0x%04X %s\n", formatId, name);

	if (!formatId)
	{
		CLIPRDR_FORMAT_DATA_RESPONSE response;

		ZeroMemory(&response, sizeof(CLIPRDR_FORMAT_DATA_RESPONSE));

		response.msgFlags = CB_RESPONSE_OK;
		response.dataLen = 0;
		response.requestedFormatData = NULL;

		cliprdr->ServerFormatDataResponse(cliprdr, &response);

		return 1;
	}

	context->requestedFormatId = formatId;

	if (strcmp(name, "CF_UNICODETEXT") == 0)
		target = _strdup("UTF8_STRING");

	fprintf(stderr, "requestedFormatAtom: %s\n", target);

	if (target)
	{
		context->requestedFormatAtom = XInternAtom(context->display, target, False);

		XConvertSelection(context->display, context->clipboard_atom,
				context->requestedFormatAtom, context->property_atom, context->window, CurrentTime);

		XFlush(context->display);
	}

	free(target);

	return 1;
}

static int freerds_cliprdr_client_format_data_response(CliprdrServerContext* cliprdr, CLIPRDR_FORMAT_DATA_RESPONSE* formatDataResponse)
{
	char* target;
	BOOL bSuccess;
	UINT32 SrcSize;
	BYTE* pSrcData;
	UINT32 DstSize;
	BYTE* pDstData;
	UINT32 formatId = 0;
	UINT32 altFormatId = 0;
	CLIPRDR_PLUGIN_CONTEXT* context = (CLIPRDR_PLUGIN_CONTEXT*) cliprdr->custom;

	fprintf(stderr, "ClientFormatDataResponse\n");

	formatId = context->serverRequestedFormatId;

	target = XGetAtomName(context->display, context->serverRequestedFormatAtom);
	altFormatId = ClipboardGetFormatId(context->system, target);
	XFree(target);

	SrcSize = (UINT32) formatDataResponse->dataLen;
	pSrcData = (BYTE*) malloc(SrcSize);

	if (!pSrcData)
		return -1;

	CopyMemory(pSrcData, formatDataResponse->requestedFormatData, SrcSize);

	bSuccess = ClipboardSetData(context->system, formatId, (void*) pSrcData, SrcSize);

	if (!bSuccess)
		free (pSrcData);

	if (bSuccess && altFormatId)
	{
		DstSize = 0;
		pDstData = (BYTE*) ClipboardGetData(context->system, altFormatId, &DstSize);
	}

	if (context->response)
	{
		XSelectionEvent* xselection = (XSelectionEvent*) context->response;

		if (xselection->property != None)
		{
			XChangeProperty(context->display, xselection->requestor,
				xselection->property, xselection->target,
				8, PropModeReplace, pDstData, DstSize);
		}

		XSendEvent(context->display, xselection->requestor, 0, 0, (XEvent*) xselection);
		XFlush(context->display);

		free(context->response);
		context->response = NULL;
	}

	free(pDstData);

	return 1;
}

static int freerds_cliprdr_server_init(CliprdrServerContext* cliprdr)
{
	UINT32 generalFlags;
	CLIPRDR_CAPABILITIES capabilities;
	CLIPRDR_MONITOR_READY monitorReady;
	CLIPRDR_GENERAL_CAPABILITY_SET generalCapabilitySet;

	ZeroMemory(&capabilities, sizeof(capabilities));
	ZeroMemory(&monitorReady, sizeof(monitorReady));

	generalFlags = 0;
	generalFlags |= CB_USE_LONG_FORMAT_NAMES;

	capabilities.msgType = CB_CLIP_CAPS;
	capabilities.msgFlags = 0;
	capabilities.dataLen = 4 + CB_CAPSTYPE_GENERAL_LEN;

	capabilities.cCapabilitiesSets = 1;
	capabilities.capabilitySets = (CLIPRDR_CAPABILITY_SET*) &generalCapabilitySet;

	generalCapabilitySet.capabilitySetType = CB_CAPSTYPE_GENERAL;
	generalCapabilitySet.capabilitySetLength = CB_CAPSTYPE_GENERAL_LEN;
	generalCapabilitySet.version = CB_CAPS_VERSION_2;
	generalCapabilitySet.generalFlags = generalFlags;

	cliprdr->ServerCapabilities(cliprdr, &capabilities);
	cliprdr->MonitorReady(cliprdr, &monitorReady);

	return 1;
}

int freerds_cliprdr_send_server_format_list(CLIPRDR_PLUGIN_CONTEXT* context)
{
	UINT32 index;
	UINT32 formatId;
	UINT32 numFormats;
	UINT32* pFormatIds;
	const char* formatName;
	CLIPRDR_FORMAT* formats;
	CLIPRDR_FORMAT_LIST formatList;
	CliprdrServerContext* cliprdr = context->cliprdr;

	ZeroMemory(&formatList, sizeof(CLIPRDR_FORMAT_LIST));

	pFormatIds = NULL;
	numFormats = ClipboardGetFormatIds(context->system, &pFormatIds);

	formats = (CLIPRDR_FORMAT*) calloc(numFormats, sizeof(CLIPRDR_FORMAT));

	if (!formats)
		return -1;

	for (index = 0; index < numFormats; index++)
	{
		formatId = pFormatIds[index];
		formatName = ClipboardGetFormatName(context->system, formatId);

		formats[index].formatId = formatId;
		formats[index].formatName = NULL;

		if ((formatId > CF_MAX) && formatName)
			formats[index].formatName = _strdup(formatName);
	}

	formatList.msgFlags = 0;
	formatList.numFormats = numFormats;
	formatList.formats = formats;

	cliprdr->ServerFormatList(cliprdr, &formatList);

	for (index = 0; index < numFormats; index++)
	{
		formatId = formats[index].formatId;
		formatName = ClipboardGetFormatName(context->system, formatId);
		fprintf(stderr, "[%d] 0x%04X %s\n", index, formatId, formatName);
		free(formats[index].formatName);
	}

	free(pFormatIds);
	free(formats);

	return 1;
}

int freerds_cliprdr_process_selection_notify(CLIPRDR_PLUGIN_CONTEXT* context, XEvent* xevent)
{
	UINT32 count;
	char** targets;
	UINT32 formatId = 0;
	CliprdrServerContext* cliprdr = context->cliprdr;
	XSelectionEvent* xselection = (XSelectionEvent*) xevent;

	fprintf(stderr, "SelectionNotify\n");

	if (xselection->target == context->targets_atom)
	{
		if (xselection->property)
		{
			targets = freerds_cliprdr_get_target_list(context, &count);

			if (freerds_cliprdr_is_target_in_list(targets, count, "text/html"))
			{
				formatId = ClipboardGetFormatId(context->system, "text/html");
			}
			else if (freerds_cliprdr_is_target_in_list(targets, count, "image/bmp"))
			{
				formatId = ClipboardGetFormatId(context->system, "image/bmp");
			}
			else if (freerds_cliprdr_is_target_in_list(targets, count, "UTF8_STRING"))
			{
				formatId = ClipboardGetFormatId(context->system, "UTF8_STRING");
			}

			if (formatId)
			{
				ClipboardSetData(context->system, formatId, NULL, 0);
			}
			else
			{
				ClipboardEmpty(context->system);
			}

			freerds_cliprdr_free_target_list(targets, count);

			freerds_cliprdr_send_server_format_list(context);
		}
		else
		{
			fprintf(stderr, "xselection->property == None\n");
		}
	}
	else
	{
		Atom type;
		BOOL bSuccess;
		UINT32 SrcSize;
		BYTE* pSrcData;
		char* type_name;
		Atom target_atom;
		BYTE* data = NULL;
		int format_property;
		unsigned long length;
		unsigned long remaining;

		target_atom = context->requestedFormatAtom;

		XGetWindowProperty(context->display, context->window,
			context->property_atom, 0, 0, 0, target_atom,
			&type, &format_property, &length, &remaining, &data);

		XFree(data);
		data = NULL;

		type_name = XGetAtomName(context->display, type);

		XGetWindowProperty(context->display, context->window,
			context->property_atom, 0, remaining, 0, target_atom,
			&type, &format_property, &length, &remaining, &data);

		if (strcmp(type_name, "UTF8_STRING") == 0)
		{
			formatId = ClipboardGetFormatId(context->system, "UTF8_STRING");

			SrcSize = (UINT32) length;
			pSrcData = (BYTE*) malloc(SrcSize + 1);

			if (!pSrcData)
				return -1;

			CopyMemory(pSrcData, data, SrcSize);
			pSrcData[SrcSize] = '\0';
			SrcSize++;

			bSuccess = ClipboardSetData(context->system, formatId, (void*) pSrcData, SrcSize);
		}

		if (formatId)
		{
			UINT32 DstSize = 0;
			BYTE* pDstData = NULL;
			CLIPRDR_FORMAT_DATA_RESPONSE response;

			ZeroMemory(&response, sizeof(CLIPRDR_FORMAT_DATA_RESPONSE));

			pDstData = (BYTE*) ClipboardGetData(context->system, context->requestedFormatId, &DstSize);

			response.msgFlags = CB_RESPONSE_OK;
			response.dataLen = DstSize;
			response.requestedFormatData = pDstData;

			cliprdr->ServerFormatDataResponse(cliprdr, &response);

			free(pDstData);

			return 1;
		}

		XFree(type_name);
		XFree(data);
	}

	return 1;
}

int freerds_cliprdr_process_selection_request(CLIPRDR_PLUGIN_CONTEXT* context, XEvent* xevent)
{
	char* target;
	XEvent* response;
	BOOL sendResponse = TRUE;
	XSelectionEvent* xselection;
	CliprdrServerContext* cliprdr = context->cliprdr;
	XSelectionRequestEvent* xselectionrequest = (XSelectionRequestEvent*) xevent;

	if (xselectionrequest->owner != context->window)
		return 1;

	response = (XEvent*) calloc(1, sizeof(XEvent));

	if (!response)
		return -1;

	xselection = (XSelectionEvent*) response;

	target = XGetAtomName(context->display, xselectionrequest->target);

	fprintf(stderr, "SelectionRequest: %s\n", target);

	xselection->property = None;
	xselection->type = SelectionNotify;
	xselection->display = xselectionrequest->display;
	xselection->requestor = xselectionrequest->requestor;
	xselection->selection = xselectionrequest->selection;
	xselection->target = xselectionrequest->target;
	xselection->time = xselectionrequest->time;

	if (xselectionrequest->target == context->timestamp_atom)
	{

	}
	else if (xselectionrequest->target == context->targets_atom)
	{
		xselection->property = xselectionrequest->property;

		if (xselection->property != None)
		{
			Atom* atoms;

			atoms = freerds_cliprdr_get_target_atom_list(context,
					context->targets, context->numTargets);

			XChangeProperty(context->display, xselection->requestor,
					xselection->property, XA_ATOM, 32, PropModeReplace,
					(BYTE*) atoms, context->numTargets);

			free(atoms);
		}
	}
	else
	{
		if (xselectionrequest->requestor != context->window)
		{
			UINT32 formatId = 0;
			CLIPRDR_FORMAT_DATA_REQUEST request;

			formatId = freerds_cliprdr_find_matching_target_format_id(context, target);

			if (formatId)
			{
				ZeroMemory(&request, sizeof(CLIPRDR_FORMAT_DATA_REQUEST));

				request.requestedFormatId = formatId;
				context->serverRequestedFormatId = formatId;
				context->serverRequestedFormatAtom = xselectionrequest->target;

				cliprdr->ServerFormatDataRequest(cliprdr, &request);

				xselection->property = xselectionrequest->property;
				context->response = response;
				sendResponse = FALSE;
			}
		}
	}

	if (sendResponse)
	{
		XSendEvent(context->display, xselection->requestor, 0, 0, (XEvent*) xselection);
		XFlush(context->display);
		free(response);
	}

	XFree(target);

	return 1;
}

int freerds_cliprdr_process_selection_clear(CLIPRDR_PLUGIN_CONTEXT* context, XEvent* xevent)
{
	fprintf(stderr, "SelectionClear\n");

	context->owner = XGetSelectionOwner(context->display, context->clipboard_atom);

	if (context->owner != context->window)
	{
		XDeleteProperty(context->display, context->root_window, context->property_atom);
	}

	return 1;
}

int freerds_cliprdr_process_property_notify(CLIPRDR_PLUGIN_CONTEXT* context, XEvent* xevent)
{
	XPropertyEvent* xproperty = (XPropertyEvent*) xevent;

	fprintf(stderr, "PropertyNotify\n");

	if (xproperty->atom != context->property_atom)
		return 1;

	return 1;
}

int freerds_cliprdr_process_xfixes_selection_notify_event(CLIPRDR_PLUGIN_CONTEXT* context, XEvent* xevent)
{
	XFixesSelectionNotifyEvent* notify = (XFixesSelectionNotifyEvent*) xevent;

	if (notify->subtype == XFixesSetSelectionOwnerNotify)
	{
		fprintf(stderr, "XFixesSetSelectionOwnerNotify\n");

		if (notify->selection != context->clipboard_atom)
			return 1;

		if (notify->owner == context->window)
			return 1;

		context->owner = notify->owner;

		/* send server format list */

		XConvertSelection(context->display, context->clipboard_atom, context->targets_atom,
				context->property_atom, context->window, notify->timestamp);
	}
	else if (notify->subtype == XFixesSelectionWindowDestroyNotify)
	{
		fprintf(stderr, "XFixesSelectionWindowDestroyNotify\n");
	}
	else if (notify->subtype == XFixesSelectionClientCloseNotify)
	{
		fprintf(stderr, "XFixesSelectionClientCloseNotify\n");
	}

	return 1;
}

int freerds_cliprdr_process_xevent(CLIPRDR_PLUGIN_CONTEXT* context, XEvent* xevent)
{
#ifdef WITH_XFIXES
	if (context->xfixes_supported)
	{
		if (xevent->type == (XFixesSelectionNotify + context->xfixes_event_base))
		{
			return freerds_cliprdr_process_xfixes_selection_notify_event(context, xevent);
		}
	}
#endif

	switch (xevent->type)
	{
		case SelectionNotify:
			freerds_cliprdr_process_selection_notify(context, xevent);
			break;

		case SelectionRequest:
			freerds_cliprdr_process_selection_request(context, xevent);
			break;

		case SelectionClear:
			freerds_cliprdr_process_selection_clear(context, xevent);
			break;

		case PropertyNotify:
			freerds_cliprdr_process_property_notify(context, xevent);
			break;

		default:
			break;
	}

	return 1;
}

int freerds_cliprdr_check_x11(CLIPRDR_PLUGIN_CONTEXT* context)
{
	XEvent xevent;

	while (XPending(context->display) > 0)
	{
		ZeroMemory(&xevent, sizeof(xevent));
		XNextEvent(context->display, &xevent);
		freerds_cliprdr_process_xevent(context, &xevent);
	}

	return 1;
}

static void* freerds_cliprdr_server_thread(CLIPRDR_PLUGIN_CONTEXT* context)
{
	DWORD status;
	DWORD nCount;
	HANDLE events[8];
	HANDLE ChannelEvent;
	CliprdrServerContext* cliprdr = context->cliprdr;

	ChannelEvent = cliprdr->GetEventHandle(cliprdr);

	nCount = 0;
	events[nCount++] = context->StopEvent;
	events[nCount++] = ChannelEvent;
	events[nCount++] = context->X11Event;

	while (1)
	{
		status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

		if (WaitForSingleObject(context->StopEvent, 0) == WAIT_OBJECT_0)
		{
			break;
		}

		if (WaitForSingleObject(ChannelEvent, 0) == WAIT_OBJECT_0)
		{
			if (cliprdr->CheckEventHandle(cliprdr) < 0)
				break;
		}

		if (WaitForSingleObject(context->X11Event, 0) == WAIT_OBJECT_0)
		{
			if (freerds_cliprdr_check_x11(context) < 0)
				break;
		}
	}

	return NULL;
}

/***************************************
 * Constructor/Destructor
 **************************************/

static CLIPRDR_PLUGIN_CONTEXT* cliprdr_plugin_context_new()
{
	CLIPRDR_PLUGIN_CONTEXT* context;

	context = (CLIPRDR_PLUGIN_CONTEXT*) calloc(1, sizeof(CLIPRDR_PLUGIN_CONTEXT));

	if (!context)
		return NULL;

	return context;
}

static void cliprdr_plugin_context_free(CLIPRDR_PLUGIN_CONTEXT *context)
{
	freerds_cliprdr_free_format_list(context->clientFormats, context->numClientFormats);

	free(context);
}


/***************************************
 * Plugin Initialization/Termination
 **************************************/

static BOOL cliprdr_plugin_on_plugin_initialize(VCPlugin* plugin)
{
	UINT32 id;
	int major, minor;
	CLIPRDR_PLUGIN_CONTEXT* context;

	context = cliprdr_plugin_context_new();

	if (!context)
		return FALSE;

	context->log = WLog_Get("freerds.channels.cliprdr");

	plugin->context = (void*) context;

	context->system = ClipboardCreate();

	context->display = XOpenDisplay(NULL);

	if (!context->display)
	{
		WLog_Print(context->log, WLOG_ERROR, "Cannot open display");
		return FALSE;
	}

	context->xfds = ConnectionNumber(context->display);
	context->X11Event = CreateFileDescriptorEvent(NULL, FALSE, FALSE, context->xfds);

	context->screen_number = DefaultScreen(context->display);
	context->screen = ScreenOfDisplay(context->display, context->screen_number);
	context->visual = DefaultVisual(context->display, context->screen_number);
	context->gc = DefaultGC(context->display, context->screen_number);
	context->depth = DefaultDepthOfScreen(context->screen);
	context->root_window = RootWindow(context->display, context->screen_number);
	context->border = BlackPixel(context->display, context->screen_number);
	context->background = WhitePixel(context->display, context->screen_number);

#ifdef WITH_XFIXES
	if (XFixesQueryExtension(context->display, &context->xfixes_event_base, &context->xfixes_error_base))
	{
		if (XFixesQueryVersion(context->display, &major, &minor))
		{
			XFixesSelectSelectionInput(context->display, context->root_window,
				context->clipboard_atom, XFixesSetSelectionOwnerNotifyMask);
			context->xfixes_supported = TRUE;
		}
	}
#endif

	context->incr_atom = XInternAtom(context->display, "INCR", False);
	context->targets_atom = XInternAtom(context->display, "TARGETS", False);
	context->timestamp_atom = XInternAtom(context->display, "TIMESTAMP", False);
	context->clipboard_atom = XInternAtom(context->display, "CLIPBOARD", False);

	context->property_atom = XInternAtom(context->display, "_FREERDS_CLIPRDR", False);
	context->identity_atom = XInternAtom(context->display, "_FREERDS_CLIPRDR_ID", False);

	context->window = XCreateSimpleWindow(context->display,
			context->root_window, 0, 0, 1, 1, 0, 0, 0);

	XChangeProperty(context->display, context->window, context->identity_atom,
				XA_INTEGER, 32, PropModeReplace, (BYTE*) &id, 1);

	XSelectInput(context->display, context->window,
			StructureNotifyMask | PropertyChangeMask);

	XFixesSelectSelectionInput(context->display, context->window,
			context->clipboard_atom,
			XFixesSetSelectionOwnerNotifyMask |
			XFixesSelectionWindowDestroyNotifyMask |
			XFixesSelectionClientCloseNotifyMask);

	return TRUE;
}

static void cliprdr_plugin_on_plugin_terminate(VCPlugin* plugin)
{
	CLIPRDR_PLUGIN_CONTEXT* context;

	context = (CLIPRDR_PLUGIN_CONTEXT*) plugin->context;

	XDestroyWindow(context->display, context->window);

	XCloseDisplay(context->display);
	CloseHandle(context->X11Event);

	ClipboardDestroy(context->system);

	cliprdr_plugin_context_free(context);
}


/***************************************
 * Terminal Service Event Handlers
 **************************************/

static void cliprdr_plugin_on_session_create(VCPlugin* plugin)
{
	CLIPRDR_PLUGIN_CONTEXT* context;

	context = (CLIPRDR_PLUGIN_CONTEXT*) plugin->context;

	if (!context)
		return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_create");
}

static void cliprdr_plugin_on_session_delete(VCPlugin* plugin)
{
	CLIPRDR_PLUGIN_CONTEXT* context;

	context = (CLIPRDR_PLUGIN_CONTEXT*) plugin->context;

	if (!context)
		return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_delete");
}

static void cliprdr_plugin_on_session_connect(VCPlugin* plugin)
{
	CLIPRDR_PLUGIN_CONTEXT* context;
	CliprdrServerContext* cliprdr;

	context = (CLIPRDR_PLUGIN_CONTEXT*) plugin->context;

	if (!context)
		return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_connect");

	if (!context->cliprdr)
	{
		cliprdr = cliprdr_server_context_new(WTS_CURRENT_SERVER_HANDLE);

		if (!cliprdr)
		{
			WLog_Print(context->log, WLOG_ERROR, "Failed to create CLIPRDR server context");
			return;
		}

		context->cliprdr = cliprdr;
		cliprdr->custom = (void*) context;

		cliprdr->ClientCapabilities = freerds_cliprdr_client_capabilities;
		cliprdr->ClientFormatList = freerds_cliprdr_client_format_list;
		cliprdr->ClientFormatListResponse = freerds_cliprdr_client_format_list_response;
		cliprdr->ClientFormatDataRequest = freerds_cliprdr_client_format_data_request;
		cliprdr->ClientFormatDataResponse = freerds_cliprdr_client_format_data_response;

		cliprdr->Open(cliprdr);
		freerds_cliprdr_server_init(cliprdr);

		context->StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		context->Thread = CreateThread(NULL, 0,
				(LPTHREAD_START_ROUTINE) freerds_cliprdr_server_thread, (void*) context, 0, NULL);
	}
}

static void cliprdr_plugin_on_session_disconnect(VCPlugin* plugin)
{
	CLIPRDR_PLUGIN_CONTEXT* context;

	context = (CLIPRDR_PLUGIN_CONTEXT*) plugin->context;

	if (!context)
		return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_disconnect");

	if (context->StopEvent)
	{
		SetEvent(context->StopEvent);
		WaitForSingleObject(context->Thread, INFINITE);
		CloseHandle(context->Thread);
		CloseHandle(context->StopEvent);
	}

	if (context->cliprdr)
	{
		cliprdr_server_context_free(context->cliprdr);
		context->cliprdr = NULL;
	}
}


/***************************************
 * Plugin Entry Point
 **************************************/

BOOL VCPluginEntry(VCPlugin* plugin)
{
	plugin->name = "CLIPRDR";

	plugin->OnPluginInitialize = cliprdr_plugin_on_plugin_initialize;
	plugin->OnPluginTerminate = cliprdr_plugin_on_plugin_terminate;

	plugin->OnSessionCreate = cliprdr_plugin_on_session_create;
	plugin->OnSessionDelete = cliprdr_plugin_on_session_delete;
	plugin->OnSessionConnect = cliprdr_plugin_on_session_connect;
	plugin->OnSessionDisconnect = cliprdr_plugin_on_session_disconnect;

	return TRUE;
}
