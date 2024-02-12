/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2023  Stefan Löffler

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	For links to further information, or to contact the authors,
	see <http://www.tug.org/texworks/>.
*/
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, const char * argv[]) {
	for (int iArg = 1; iArg < argc; ++iArg) {
		const char * arg = argv[iArg];
		const size_t argLen = std::strlen(arg);
		for (size_t iChar = 0; iChar < argLen; ++iChar) {
			if (arg[iChar] == '\\') {
				++iChar;
				if (arg[iChar] == 'x') {
					unsigned int code{0};
					if (sscanf(&arg[iChar], "x%02x", &code) != 1) {
						fprintf(stderr, "Error processing argument %i at position %zu\n", iArg, iChar);
						return 1;
					}
					iChar += 2;
					printf("%c", static_cast<char>(code));
					continue;
				}
				else {
					fprintf(stderr, "Unknown escape sequence in argument %i at position %zu\n", iArg, iChar);
					return 1;
				}
			}
			else {
				printf("%c", arg[iChar]);
			}
		}
	}
	return 0;
}
