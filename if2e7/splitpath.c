void splitpath(const char* path, char* drv, char* dir, char* name, char* ext)
{
/*
	* Copyright 2000, 2004 Martin Fuchs
	*
	* This library is free software; you can redistribute it and/or
	* modify it under the terms of the GNU Lesser General Public
	* License as published by the Free Software Foundation; either
	* version 2.1 of the License, or (at your option) any later version.
	*
	* This library is distributed in the hope that it will be useful,
	* but WITHOUT ANY WARRANTY; without even the implied warranty of
	* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	* Lesser General Public License for more details.
	*
	* You should have received a copy of the GNU Lesser General Public
	* License along with this library; if not, write to the Free Software
	* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
*/

	const char* end; /* end of processed string */
	const char* p;   /* search pointer */
	const char* s;   /* copy pointer */

	/* extract drive name */
	if (path[0] && path[1]==':') {
		if (drv) {
			*drv++ = *path++;
			*drv++ = *path++;
			*drv = '\0';
		}
	} else if (drv)
		*drv = '\0';

	/* search for end of string or stream separator */
	for(end=path; *end && *end!=':'; )
		end++;

	/* search for begin of file extension */
	for(p=end; p>path && *--p!='\\' && *p!='/'; )
		if (*p == '.') {
			end = p;
			break;
		}

	if (ext)
		for(s=end; (*ext=*s++); )
			ext++;

	/* search for end of directory name */
	for(p=end; p>path; )
		if (*--p=='\\' || *p=='/') {
			p++;
			break;
		}

	if (name) {
		for(s=p; s<end; )
			*name++ = *s++;

		*name = '\0';
	}

	if (dir) {
		for(s=path; s<p; )
			*dir++ = *s++;

		*dir = '\0';
	}
}
