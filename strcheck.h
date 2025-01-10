#include <string.h>

extern inline int is_alphanumeric(char *str) {
	for (int i = 0; i < strlen(str); i++) {
		if (
			(str[i] < '0' || str[i] > '9') &&
			(str[i] < 'a' || str[i] > 'z') &&
			(str[i] < 'A' || str[i] > 'Z')
		) {
			return 0;
		}
	}
	return 1;
}
