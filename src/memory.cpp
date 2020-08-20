#include "memory.h"

Memory::Memory(uint32_t size) {
    this->size = size;
}

Memory::~Memory() {
    for (auto i : this->table) {
        delete i;
    }
}

void Memory::map(uint32_t start, uint32_t end, uint8_t *data,
    uint32_t size, bool readable, bool writeable) {
    if (!(0 <= start && start < end && end <= this->size)) {
        error(format("Memory: the range from start to end has to be satisfied [0, %ld]", this->size));
    }
    if ((end - start) % size != 0) {
        error(format("Memory: the range of (end - start) must be an integer multiple of %ld", this->size));
    }
    Blank *item = new Blank{ data, size, start, end, nullptr, nullptr, readable, writeable };
    this->add(item);
}

void Memory::map(uint32_t start, uint32_t end, ReadCallBack rCallBack, WriteCallBack wCallBack,
    uint32_t size, bool readable, bool writeable) {
    if (!(0 <= start && start < end && end <= this->size)) {
        error(format("Memory: the range from start to end has to be satisfied [0, %ld]", this->size));
    }
    if ((end - start) % size != 0) {
        error(format("Memory: the range of (end - start) must be an integer multiple of %ld", this->size));
    }
    Blank *item = new Blank{ nullptr, size, start, end, rCallBack, wCallBack, readable, writeable };
    this->add(item);
}

uint8_t Memory::read(uint32_t addr) {
    CHECKADDR(addr);
    uint32_t a = addr;
    Blank* blank = this->findBlank(addr);
    if (blank != nullptr) {
        if (blank->readable) {
            if (blank->data) {
                return blank->data[addr];
            } else {
                return blank->rCallBack(addr);
            }
        }
        error(format("Memory: address(%0#6x) cannot be read!", a));
    }
    //printf("Memory: address(%0#6x) is unmapped!\n", a);
    //exit(0);
    return 0;
}

void Memory::write(uint32_t addr, uint8_t byte) {
    CHECKADDR(addr);
    uint32_t a = addr;
    Blank* blank = this->findBlank(addr);
    if (blank != nullptr) {
        if (blank->writeable) {
            if (blank->data) {
                blank->data[addr] = byte;
                return;
            } else {
                blank->wCallBack(addr, byte);
                return;
            }
        }
        error(format("Memory: address(%0#6x) cannot be written!", a));
    }
    //printf("Memory: address(%0#6x) is unmapped!\n", a);
    //exit(0);
}

void Memory::add(Blank *item) {
    for (auto i = this->table.begin(); i != this->table.end(); i++) {
        Blank *j = *i;
        if ((j->start <= item->start && item->end <= j->end)
            || (j->start <= item->start && item->start < j->end)
            || (j->start < item->end && item->end <= j->end)) {
            error("Memory: the range start to end intersects with the mapped region");
        }
        if (item->end <= j->start) {
            this->table.insert(i, item);
            return;
        }
    }
    this->table.push_back(item);
}

Blank* Memory::findBlank(uint32_t& addr) {
    for (auto i : this->table) {
        if (i->start <= addr && addr < i->end) {
            addr = (addr - i->start) % i->size;
            return i;
        }
    }
    return nullptr;
}