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
