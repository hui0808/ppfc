#ifndef __PPFC_UTILS_H__
#define __PPFC_UTILS_H__

#include <string>
#include <iostream>

#define READBIND(func) [this](uint32_t addr) { return func(addr); }
#define WRITEBIND(func) [this](uint32_t addr, uint8_t byte) { func(addr, byte); }
#define EVENTBIND(func) [this](SDL_Event& e) { func(e); }

template<class T>
void _print(T arg) {
	std::cout << arg << " ";
}

template<class... Args>
void log(Args... args) {
	int arr[] = { (_print(args), 0)... };
	std::cout << std::endl;
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

template<class... Args>
void ensure_(bool condition, const char* file, const char* func, int line, Args... message) {
	if (not condition) {
		log("*** ≤‚ ‘ ß∞‹: ", message..., format("%s:%s:%d", file, func, line));
	} else {
		log("||| ≤‚ ‘≥…π¶");
	}
}

#define ensure(condition, ...) \
do{ \
    if (!(condition)) { \
        log("*** ≤‚ ‘ ß∞‹: ", ##__VA_ARGS__, format("%s:%s:%d", __FILE__, __func__, __LINE__)); \
    } else { \
        log("||| ≤‚ ‘≥…π¶"); \
    } \
}while(0)

#define error(...) \
do{ \
    log(##__VA_ARGS__, format("%s:%s:%d", __FILE__, __func__, __LINE__)); \
    exit(1); \
}while(0)

#define assert(condition, ...) \
do{ \
    if (!(condition)) { \
        log(##__VA_ARGS__, format("%s:%s:%d", __FILE__, __func__, __LINE__)); \
        exit(1); \
    } \
}while(0)

#endif // __PPFC_UTILS_H__
