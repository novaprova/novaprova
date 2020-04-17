#define NOPS8 \
    __asm__("nop"); \
    __asm__("nop"); \
    __asm__("nop"); \
    __asm__("nop"); \
    __asm__("nop"); \
    __asm__("nop"); \
    __asm__("nop"); \
    __asm__("nop")

#define NOPS256 \
    NOPS8; \
    NOPS8; \
    NOPS8; \
    NOPS8; \
    NOPS8; \
    NOPS8; \
    NOPS8; \
    NOPS8

#define NOPS2048 \
    NOPS256; \
    NOPS256; \
    NOPS256; \
    NOPS256; \
    NOPS256; \
    NOPS256; \
    NOPS256; \
    NOPS256

#define NOPS16384 \
    NOPS2048; \
    NOPS2048; \
    NOPS2048; \
    NOPS2048; \
    NOPS2048; \
    NOPS2048; \
    NOPS2048; \
    NOPS2048

#define NOPS131072 \
    NOPS16384; \
    NOPS16384; \
    NOPS16384; \
    NOPS16384; \
    NOPS16384; \
    NOPS16384; \
    NOPS16384; \
    NOPS16384

void nopfunc(void)
{
    NOPS16384;
}
