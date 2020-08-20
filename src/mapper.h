#ifndef __PPFC_MAPPER_H__
#define __PPFC_MAPPER_H__

#include "common.h"

class PPFC;

class Mapper {
public:
    PPFC& bus;
    uint8_t mapper;

    Mapper(PPFC& bus);
    void init(void);
    void mapper0(void);

};

#endif // __PPFC_MAPPER_H__
