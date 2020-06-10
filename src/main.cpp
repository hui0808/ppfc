//#define CPULOG
#include "ppfc.h"

int main(int argc, char *argv[]) {
	if (argc == 2) {
		PPFC fc(argv[1]);
		fc.run();
	} else {
		error("ppfc: arg error, expected nes file path!");
	}
	return 0;
}