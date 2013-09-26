/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * xrdp-ng interprocess communication protocol
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

#include <xrdp-ng/xrdp.h>

#include "transport.h"

int freerds_named_pipe_read(HANDLE hNamedPipe, BYTE* data, DWORD length)
{
	BOOL fSuccess = FALSE;
	DWORD NumberOfBytesRead;
	DWORD TotalNumberOfBytesRead = 0;

	NumberOfBytesRead = 0;

	fSuccess = ReadFile(hNamedPipe, data, length, &NumberOfBytesRead, NULL);

	if (!fSuccess || (NumberOfBytesRead == 0))
	{
		return -1;
	}

	TotalNumberOfBytesRead += NumberOfBytesRead;
	length -= NumberOfBytesRead;
	data += NumberOfBytesRead;

	return TotalNumberOfBytesRead;
}

int freerds_named_pipe_write(HANDLE hNamedPipe, BYTE* data, DWORD length)
{
	BOOL fSuccess = FALSE;
	DWORD NumberOfBytesWritten;
	DWORD TotalNumberOfBytesWritten = 0;

	while (length > 0)
	{
		NumberOfBytesWritten = 0;

		fSuccess = WriteFile(hNamedPipe, data, length, &NumberOfBytesWritten, NULL);

		if (!fSuccess || (NumberOfBytesWritten == 0))
		{
			return -1;
		}

		TotalNumberOfBytesWritten += NumberOfBytesWritten;
		length -= NumberOfBytesWritten;
		data += NumberOfBytesWritten;
	}

	return NumberOfBytesWritten;
}
