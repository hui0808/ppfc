#ifndef __PPFC_CARTRIDGE_H__
#define __PPFC_CARTRIDGE_H__

#include "common.h"

struct CartridgeHeader {
	uint32_t id;
	uint8_t  prgromCount;
	uint8_t  chrromCount;
	uint8_t  flag1;
	uint8_t  flag2;
	uint8_t  reserved[8];
};

union CartridgeFlag1 {
	uint8_t flag;
	struct {
		bool mirroring : 1;
		bool sram : 1;
		bool trainer : 1;
		bool fourScreen : 1;
		uint8_t lmapper : 4;
	};
};

union CartridgeFlag2 {
	uint8_t flag;
	struct {
		uint8_t consoleType : 2;
		uint8_t version : 2;
		uint8_t hmapper : 4;
	};
};

class Cartridge {
public:
	Cartridge(const char* path);
	~Cartridge();
	void loadHeader(void);
	void loadFlags(void);
	void loadRom(void);

	const char* path;
	FILE* file;
	CartridgeHeader header;
	CartridgeFlag1 flag1;
	CartridgeFlag2 flag2;
	uint8_t mapper;
	uint8_t *prgrom;
	uint8_t *chrrom;
	uint8_t *saveram;
};

#endif // __PPFC_CARTRIDGE_H__
