/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2012
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
 *
 * simple list
 */

#include "os_calls.h"
#include "list.h"

#include <winpr/crt.h>

xrdpList* list_create(void)
{
	xrdpList *self;

	self = (xrdpList *) g_malloc(sizeof(xrdpList), 1);
	self->grow_by = 10;
	self->alloc_size = 10;
	self->items = (LONG_PTR *) g_malloc(sizeof(LONG_PTR) * 10, 1);
	return self;
}

void list_delete(xrdpList *self)
{
	int i;

	if (self == 0)
	{
		return;
	}

	if (self->auto_free)
	{
		for (i = 0; i < self->count; i++)
		{
			free((void *) self->items[i]);
			self->items[i] = 0;
		}
	}

	free(self->items);
	free(self);
}

void list_add_item(xrdpList *self, LONG_PTR item)
{
	LONG_PTR *p;
	int i;

	if (self->count >= self->alloc_size)
	{
		i = self->alloc_size;
		self->alloc_size += self->grow_by;
		p = (LONG_PTR *) g_malloc(sizeof(LONG_PTR) * self->alloc_size, 1);
		g_memcpy(p, self->items, sizeof(LONG_PTR) * i);
		free(self->items);
		self->items = p;
	}

	self->items[self->count] = item;
	self->count++;
}

LONG_PTR list_get_item(xrdpList *self, int index)
{
	if (index < 0 || index >= self->count)
	{
		return 0;
	}

	return self->items[index];
}

void list_clear(xrdpList *self)
{
	int i;

	if (self->auto_free)
	{
		for (i = 0; i < self->count; i++)
		{
			free((void *) self->items[i]);
			self->items[i] = 0;
		}
	}

	free(self->items);
	self->count = 0;
	self->grow_by = 10;
	self->alloc_size = 10;
	self->items = (LONG_PTR *) g_malloc(sizeof(LONG_PTR) * 10, 1);
}

int list_index_of(xrdpList *self, LONG_PTR item)
{
	int i;

	for (i = 0; i < self->count; i++)
	{
		if (self->items[i] == item)
		{
			return i;
		}
	}

	return -1;
}

void list_remove_item(xrdpList *self, int index)
{
	int i;

	if (index >= 0 && index < self->count)
	{
		if (self->auto_free)
		{
			free((void *) self->items[index]);
			self->items[index] = 0;
		}

		for (i = index; i < (self->count - 1); i++)
		{
			self->items[i] = self->items[i + 1];
		}

		self->count--;
	}
}

void list_insert_item(xrdpList *self, int index, LONG_PTR item)
{
	LONG_PTR *p;
	int i;

	if (index == self->count)
	{
		list_add_item(self, item);
		return;
	}

	if (index >= 0 && index < self->count)
	{
		self->count++;

		if (self->count > self->alloc_size)
		{
			i = self->alloc_size;
			self->alloc_size += self->grow_by;
			p = (LONG_PTR *) g_malloc(sizeof(LONG_PTR) * self->alloc_size, 1);
			g_memcpy(p, self->items, sizeof(LONG_PTR) * i);
			free(self->items);
			self->items = p;
		}

		for (i = (self->count - 2); i >= index; i--)
		{
			self->items[i + 1] = self->items[i];
		}

		self->items[index] = item;
	}
}

/* append one list to another using strdup for each item in the list */
/* begins copy at start_index, a zero based index on the soure list */
void list_append_list_strdup(xrdpList *self, xrdpList *dest, int start_index)
{
	int index;
	LONG_PTR item;
	char *dup;

	for (index = start_index; index < self->count; index++)
	{
		item = list_get_item(self, index);
		dup = g_strdup((char *) item);
		list_add_item(dest, (LONG_PTR) dup);
	}
}

void list_dump_items(xrdpList *self)
{
	int index;

	if (self->count == 0)
	{
		g_writeln("List is empty");
	}

	for (index = 0; index < self->count; index++)
	{
		g_writeln("%d: %s", index, list_get_item(self, index));
	}
}
