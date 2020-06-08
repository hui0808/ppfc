#ifndef __PPFC_MEMORY_H__
#define __PPFC_MEMORY_H__

#include "common.h"

using ReadCallBack = std::function<uint8_t(uint32_t)>;
using WriteCallBack = std::function<void(uint32_t, uint8_t)>;

#define CHECKADDR(a) assert(0 <= (a) && (a) < this->size, format("Memory: address(%0#6x) out of [0, %0#6x)", (addr), this->size))

struct Blank {
	uint8_t *data;
	uint32_t size;
	uint32_t start;
	uint32_t end;
	ReadCallBack rCallBack;
	WriteCallBack wCallBack;
	bool readable;
	bool writeable;
};

class Memory {
public:
	uint32_t size;
	std::vector<Blank*>table;

	Memory(uint32_t size);
	~Memory();
	void map(uint32_t start, uint32_t end, uint8_t *data,
		uint32_t size, bool readable = true, bool writeable = true);
	void map(uint32_t start, uint32_t end, ReadCallBack rCallBack, WriteCallBack wCallBack, 
		uint32_t size, bool readable = true, bool writeable = true);

	uint8_t read(uint32_t addr);
	void write(uint32_t addr, uint8_t byte);
	uint16_t readw(uint32_t addr);
	void writew(uint32_t addr, uint16_t byte);

private:
	void add(Blank *item);
	Blank* findBlank(uint32_t& addr);
};

inline uint16_t Memory::readw(uint32_t addr) {
	uint16_t l = this->read(addr);
	uint16_t h = this->read(addr + 1);
	return l | (h << 8);
}

inline void Memory::writew(uint32_t addr, uint16_t byte) {
	this->write(addr, uint8_t(byte));
	this->write(addr + 1, byte >> 8);
}

#endif // __PPFC_MEMORY_H__
