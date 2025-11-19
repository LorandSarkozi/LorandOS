;;-----------------_DEFINITIONS ONLY_-----------------------
;; IMPORT FUNCTIONS FROM C
%macro IMPORTFROMC 1-*
%rep  %0
    %ifidn __OUTPUT_FORMAT__, win32 ; win32 builds from Visual C decorate C names using _ 
    extern _%1
    %1 equ _%1
    %else
    extern %1
    %endif
%rotate 1
%endrep
%endmacro

;; EXPORT TO C FUNCTIONS
%macro EXPORT2C 1-*
%rep  %0
    %ifidn __OUTPUT_FORMAT__, win32 ; win32 builds from Visual C decorate C names using _ 
    global _%1
    _%1 equ %1
    %else
    global %1
    %endif
%rotate 1
%endrep
%endmacro

%define break xchg bx, bx

IMPORTFROMC KernelMain

TOP_OF_STACK                equ 0x200000   ; 2 MiB
KERNEL_BASE_PHYSICAL        equ 0x200000
;;-----------------^DEFINITIONS ONLY^-----------------------

; ---------- Flags ----------
%define P   1
%define RW  (1<<1)
%define PT_FLAGS   (P|RW)
%define PDE_FLAGS  (P|RW)
%define PDP_FLAGS  (P|RW)
%define PML4_FLAGS (P|RW)

%macro SET_QW 3
    mov     eax, %2
    or      eax, %3
    mov     dword [%1], eax
    mov     dword [%1+4], 0
%endmacro


segment .text
[BITS 32]
ASMEntryPoint:
    cli
    MOV     DWORD [0x000B8000], 'O1S1'
%ifidn __OUTPUT_FORMAT__, win32
    MOV     DWORD [0x000B8004], '3121'                  ; 32 bit build marker
%else
    MOV     DWORD [0x000B8004], '6141'                  ; 64 bit build marker
%endif

    mov     esp, TOP_OF_STACK

    break 

    ; ============================
    ; [0..4MiB]
    ; PML4[0] -> PDPT_ID
    ; PDPT_ID[0] -> PD_ID
    ; PD_ID[0] -> PT_ID0 (0..2MiB)
    ; PD_ID[1] -> PT_ID1 (2..4MiB)
    ; ============================

    ; PML4[0] -> PDPT_ID
    lea     edi, [PML4]
    mov     eax, PDPT_ID
    and     eax, 0xFFFFF000
    SET_QW  edi, eax, PML4_FLAGS

    ; PDPT_ID[0] -> PD_ID
    lea     edi, [PDPT_ID]
    mov     eax, PD_ID
    and     eax, 0xFFFFF000
    SET_QW  edi, eax, PDP_FLAGS

    ; PD_ID[0] -> PT_ID0
    lea     edi, [PD_ID]
    mov     eax, PT_ID0
    and     eax, 0xFFFFF000
    SET_QW  edi, eax, PDE_FLAGS

    ; PD_ID[1] -> PT_ID1
    lea     edi, [PD_ID + 8*1]
    mov     eax, PT_ID1
    and     eax, 0xFFFFF000
    SET_QW  edi, eax, PDE_FLAGS

    ; PT_ID0: 0..2MiB (512 entries)
    lea     edi, [PT_ID0]
    mov     ecx, 512
    xor     ebx, ebx              ; frame = 0
.pt0:
    mov     eax, ebx
    or      eax, PT_FLAGS
    mov     [edi], eax
    mov     dword [edi+4], 0
    add     ebx, 0x1000
    add     edi, 8
    loop    .pt0

    ; PT_ID1: 2..4MiB
    lea     edi, [PT_ID1]
    mov     ecx, 512
    mov     ebx, 0x00200000
.pt1:
    mov     eax, ebx
    or      eax, PT_FLAGS
    mov     [edi], eax
    mov     dword [edi+4], 0
    add     ebx, 0x1000
    add     edi, 8
    loop    .pt1

    ; ============================
    ; Enable paging & Long mode
    ; ============================
    mov     eax, PML4
    mov     cr3, eax

    mov     eax, cr4
    or      eax, (1<<5)                ; PAE
    or      eax, (1<<9)|(1<<10)        ; OSFXSR|OSXMMEXCPT (SSE safe)
    mov     cr4, eax

    mov     ecx, 0xC0000080            ; IA32_EFER
    rdmsr
    or      eax, (1<<8)                ; LME
    wrmsr

    lgdt    [gdt_ptr]

    mov     eax, cr0
    or      eax, (1<<31)               ; PG
    mov     cr0, eax

    jmp     0x08:LongModeEntry         ; far jump in 64-bit

; ---------------- 64-bit land ----------------
[BITS 64]
LongModeEntry:
    mov     ax, 0x10                   ; data selector
    mov     ds, ax
    mov     es, ax
    mov     ss, ax
    mov     fs, ax
    mov     gs, ax
	
	call    InitIDT    

    extern KernelMain
    call    KernelMain

.hang:
    cli
    hlt
    jmp     .hang

;;--------------------------------------------------------

[BITS 32]
__cli:      CLI    
            RET
			
__sti:      STI
            RET
			
__magic:    XCHG BX,BX
            RET
			
__enableSSE:			 ;; enable SSE instructions (CR4.OSFXSR = 1)  
    MOV     EAX, CR4 
    OR      EAX, 0x00000200
    MOV     CR4, EAX
    RET

EXPORT2C ASMEntryPoint, __cli, __sti, __magic, __enableSSE

; =======================
; GDT 64-bit
; =======================
section .data align=16
gdt64:
    dq 0x0000000000000000          ; null
    dq 0x00AF9A000000FFFF          ; 0x08: code64 (L=1, D=0)
    dq 0x00AF92000000FFFF          ; 0x10: data
gdt_ptr:
    dw gdt_end - gdt64 - 1
    dd gdt64
gdt_end:

; =======================
; Paging tables
; =======================
align 4096
PML4:       times 512 dq 0

align 4096
PDPT_ID:    times 512 dq 0

align 4096
PD_ID:      times 512 dq 0

align 4096
PT_ID0:     times 512 dq 0

align 4096
PT_ID1:     times 512 dq 0


; ================== IDT + ISR STUBS ==================
[BITS 64]
DEFAULT REL
extern  IsrDispatch           ; for the new file I will create to print out the registers(also to see if error / no error code)

%macro PUSH_ALL 0
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rsi
    push rdi
    push rbp
    push rdx
    push rcx
    push rbx
    push rax
%endmacro

; --------- stubs (no error code) ----------
%macro ISR_NOEC 2
global %2
%2:
    sub     rsp, 8
    mov     qword [rsp], 0         ; fake EC=0
    mov     r9, [rsp+8]            ; RIP for the C side
    PUSH_ALL
    mov     ecx, %1                ; RCX = vector
    xor     edx, edx               ; RDX = error code
    mov     r8,  rsp               ; R8  = &saved regs
    sub     rsp, 32                ; shadow space
    call    IsrDispatch
    add     rsp, 32
.hlt_%2:
    hlt
    jmp     .hlt_%2
%endmacro

; --------- stubs (with error code) ----------
%macro ISR_EC 2
global %2
%2:
    mov     r9, [rsp+8]            ; RIP (EC at [rsp+0])
    PUSH_ALL
    mov     rdx, [rsp+120]         ; RDX = EC (15 regs * 8)
    mov     ecx, %1                ; RCX = vector
    mov     r8,  rsp               ; R8  = &saved regs
    sub     rsp, 32
    call    IsrDispatch
    add     rsp, 32
.hlt_%2:
    hlt
    jmp     .hlt_%2
%endmacro

; ---------- instantiate 0..31 ----------
ISR_NOEC 0,  isr0      ; #DE
ISR_NOEC 1,  isr1
ISR_NOEC 2,  isr2
ISR_NOEC 3,  isr3
ISR_NOEC 4,  isr4
ISR_NOEC 5,  isr5
ISR_NOEC 6,  isr6      ; #UD (no EC)
ISR_NOEC 7,  isr7
ISR_EC   8,  isr8      ; #DF (pushes 0 EC)
ISR_NOEC 9,  isr9
ISR_EC   10, isr10     ; #TS
ISR_EC   11, isr11     ; #NP
ISR_EC   12, isr12     ; #SS
ISR_EC   13, isr13     ; #GP
ISR_EC   14, isr14     ; #PF
ISR_NOEC 15, isr15
ISR_NOEC 16, isr16
ISR_EC   17, isr17     ; #AC
ISR_NOEC 18, isr18
ISR_NOEC 19, isr19
ISR_NOEC 20, isr20
ISR_EC   21, isr21     ; #CP
ISR_NOEC 22, isr22
ISR_NOEC 23, isr23
ISR_NOEC 24, isr24
ISR_NOEC 25, isr25
ISR_NOEC 26, isr26
ISR_NOEC 27, isr27
ISR_EC   28, isr28     ; #HV
ISR_NOEC 29, isr29
ISR_NOEC 30, isr30
ISR_NOEC 31, isr31

; --------- IRQ handlers (no error code, no halt) ----------
extern IrqDispatch

%macro IRQ_HANDLER 2
global %2
%2:
    sub     rsp, 8
    mov     qword [rsp], 0         ; fake EC=0
    PUSH_ALL
    mov     ecx, %1                ; RCX = IRQ number (0-15)
    mov     r8,  rsp               ; R8  = &saved regs
    sub     rsp, 32                ; shadow space
    call    IrqDispatch
    add     rsp, 32
    add     rsp, 8*15              ; pop all saved regs
    add     rsp, 8                 ; pop fake EC
    iretq
%endmacro

; IRQ 0-15 mapped to vectors 32-47
IRQ_HANDLER 0,  irq0     ; PIT Timer
IRQ_HANDLER 1,  irq1     ; Keyboard
IRQ_HANDLER 2,  irq2     ; Cascade
IRQ_HANDLER 3,  irq3     ; COM2
IRQ_HANDLER 4,  irq4     ; COM1
IRQ_HANDLER 5,  irq5     ; LPT2
IRQ_HANDLER 6,  irq6     ; Floppy
IRQ_HANDLER 7,  irq7     ; LPT1
IRQ_HANDLER 8,  irq8     ; RTC
IRQ_HANDLER 9,  irq9     ; Free
IRQ_HANDLER 10, irq10    ; Free
IRQ_HANDLER 11, irq11    ; Free
IRQ_HANDLER 12, irq12    ; PS/2 Mouse
IRQ_HANDLER 13, irq13    ; FPU
IRQ_HANDLER 14, irq14    ; Primary ATA
IRQ_HANDLER 15, irq15    ; Secondary ATA

; ---------- build & load IDT ----------
%define IDT_TYPE  0x8E    ; present, DPL0, interrupt gate
%define KCODE_SEL 0x08    ; your 64-bit code selector

%macro SET_IDT 2
    ; %1 = vector, %2 = handler
    lea     rax, [rel %2]             ; rax = handler address (RIP-relative)
    lea     rdx, [rel IDT]            ; rdx = &IDT[0]
    lea     rdx, [rdx + %1*16]        ; rdx = &IDT[%1]

    mov     word  [rdx+0], ax         ; offset low
    mov     word  [rdx+2], 0x08       ; selector (your 64-bit code selector)
    mov     byte  [rdx+4], 0          ; IST = 0
    mov     byte  [rdx+5], 0x8E       ; present, DPL0, interrupt gate

    shr     rax, 16
    mov     word  [rdx+6], ax         ; offset mid
    shr     rax, 16
    mov     dword [rdx+8], eax        ; offset high
    mov     dword [rdx+12], 0         ; zero
%endmacro

global InitIDT
InitIDT:
    ; fill 0..31
    SET_IDT 0,  isr0
    SET_IDT 1,  isr1
    SET_IDT 2,  isr2
    SET_IDT 3,  isr3
    SET_IDT 4,  isr4
    SET_IDT 5,  isr5
    SET_IDT 6,  isr6
    SET_IDT 7,  isr7
    SET_IDT 8,  isr8
    SET_IDT 9,  isr9
    SET_IDT 10, isr10
    SET_IDT 11, isr11
    SET_IDT 12, isr12
    SET_IDT 13, isr13
    SET_IDT 14, isr14
    SET_IDT 15, isr15
    SET_IDT 16, isr16
    SET_IDT 17, isr17
    SET_IDT 18, isr18
    SET_IDT 19, isr19
    SET_IDT 20, isr20
    SET_IDT 21, isr21
    SET_IDT 22, isr22
    SET_IDT 23, isr23
    SET_IDT 24, isr24
    SET_IDT 25, isr25
    SET_IDT 26, isr26
    SET_IDT 27, isr27
    SET_IDT 28, isr28
    SET_IDT 29, isr29
    SET_IDT 30, isr30
    SET_IDT 31, isr31
    
    ; fill 32..47 (IRQs)
    SET_IDT 32, irq0
    SET_IDT 33, irq1
    SET_IDT 34, irq2
    SET_IDT 35, irq3
    SET_IDT 36, irq4
    SET_IDT 37, irq5
    SET_IDT 38, irq6
    SET_IDT 39, irq7
    SET_IDT 40, irq8
    SET_IDT 41, irq9
    SET_IDT 42, irq10
    SET_IDT 43, irq11
    SET_IDT 44, irq12
    SET_IDT 45, irq13
    SET_IDT 46, irq14
    SET_IDT 47, irq15

 ; load IDTR 
	lea     rax, [rel IDT]
	mov     word  [rel IDTR_limit], (256*16 - 1)
	mov     qword [rel IDTR_base],  rax
	lidt    [rel IDTR_limit]
	ret

; ---------- test helpers(I just wanna see if everything works, thats optional) ----------
global TriggerUD2
TriggerUD2:
    ud2
    ret

global TriggerPF
TriggerPF:
    mov     rax, 0xFFFF800000000000     ; unmapped
    mov     qword [rax], 0              ; #PF
    ret

section .data align=16
IDT:    times 256 dq 0,0

IDTR:
IDTR_limit: dw 0
IDTR_base:  dq 0