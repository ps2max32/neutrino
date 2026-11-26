/* Part of neutrino: the EE side without an SDK. See ../README.md.
   A program at 0x100000. The main thread's stack is at the end of memory
   (SetupThread with -1), the heap up to it. */
ENTRY(_start)

SECTIONS {
    . = 0x00100000;
    .text : {
        *(.text._start)
        *(.text .text.*)
    }
    .rodata ALIGN(16) : { *(.rodata .rodata.*) }
    .data ALIGN(16) : {
        *(.data .data.*)
        *(.sdata .sdata.*)
    }
    _gp = ALIGN(16) + 0x7ff0;
    .bss ALIGN(64) : {
        _fbss = .;
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(16);
    }
    _end = .;

    _stack      = -1;
    _stack_size = 0x20000;
    _heap_end   = 0x02000000 - _stack_size;

    /DISCARD/ : { *(.MIPS.abiflags) *(.reginfo) *(.pdr) *(.comment) *(.gnu.attributes) *(.mdebug.*) }
}
