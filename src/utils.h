#ifndef __PPFC_UTILS_H__
#define __PPFC_UTILS_H__

#include <string>
#include <iostream>

template<class T>
void _print(T arg) {
	std::cout << arg << " ";
}

template<class... Args>
void log(Args... args) {
	int arr[] = { (_print(args), 0)... };
	if (sizeof(arr) != 0) {
		std::cout << std::endl;
	}
}

template<class... Args>
std::string format(const char* format_, Args... args) {
	int size = std::snprintf(nullptr, 0, format_, args...);
	size++;
	char* buf = new char[size];
	std::snprintf(buf, size, format_, args...);
	std::string str(buf);
	delete[] buf;
	return str;
}

#define error(info, ...) \
do{ \
	printf("%s:%s:%d: ", __FILE__, __func__, __LINE__); \
    log(info, ##__VA_ARGS__); \
    exit(1); \
}while(0)

#define assert(condition, ...) \
do{ \
    if (!(condition)) { \
		printf("%s:%s:%d: assert", __FILE__, __func__, __LINE__); \
        log("", ##__VA_ARGS__); \
        exit(1); \
    } \
}while(0)

#endif // __PPFC_UTILS_H__
