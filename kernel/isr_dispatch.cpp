#include <stdint.h>
#include <intrin.h>

#pragma pack(push,1)
struct CpuState {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rsi, rdi, rbp, rdx, rcx, rbx, rax;
    uint16_t cs, ss, ds, es;
    uint64_t rflags, rip, rsp;
};
#pragma pack(pop)

static volatile uint16_t* const VGA = (uint16_t*)0xB8000;
static int row = 0;
static void vga_puts(const char* s) { uint16_t a = 0x0F << 8; int c = 0; for (; *s && row < 25; ++s) { if (*s == '\n') { row++; c = 0; continue; } VGA[row * 80 + c] = (uint16_t)(*s) | a; if (++c >= 80) { row++; c = 0; } } }
static void hex64(const char* n, uint64_t v) { static const char H[] = "0123456789ABCDEF"; char b[64], * p = b; while (*n)*p++ = *n++; *p++ = '='; *p++ = '0'; *p++ = 'x'; for (int i = 60; i >= 0; i -= 4)*p++ = H[(v >> i) & 0xF]; *p++ = '\n'; *p = 0; vga_puts(b); }
static void hex16(const char* n, uint16_t v) { static const char H[] = "0123456789ABCDEF"; char b[32], * p = b; while (*n)*p++ = *n++; *p++ = '='; *p++ = '0'; *p++ = 'x'; for (int i = 12; i >= 0; i -= 4)*p++ = H[(v >> i) & 0xF]; *p++ = '\n'; *p = 0; vga_puts(b); }

extern "C" void IsrDispatch(uint8_t vec, uint64_t ec, CpuState* s, uint64_t rip_from_stub)
{
    s->cs = 0x08;    
    s->ss = 0x10;    
    s->ds = 0x00;    
    s->es = 0x00;

    s->rflags = __readeflags();
    s->rip = rip_from_stub;
    s->rsp = (uint64_t)_AddressOfReturnAddress();

    vga_puts("=== EXC ===\n");
    hex16("VEC  ", vec);
    hex64("EC   ", ec);
    hex64("RIP  ", s->rip);
    hex64("RFLAGS", s->rflags);
    hex16("CS   ", s->cs); hex16("SS", s->ss);

    hex64("RAX  ", s->rax);  hex64("RBX", s->rbx);
    hex64("RCX  ", s->rcx);  hex64("RDX", s->rdx);
    hex64("RSI  ", s->rsi);  hex64("RDI", s->rdi);
    hex64("RBP  ", s->rbp);  hex64("RSP", s->rsp);
    hex64("R8   ", s->r8);   hex64("R9 ", s->r9);
    hex64("R10  ", s->r10);  hex64("R11", s->r11);
    hex64("R12  ", s->r12);  hex64("R13", s->r13);
    hex64("R14  ", s->r14);  hex64("R15", s->r15);
    vga_puts("HALT\n");

    __halt();  
    for (;;) __halt();
}