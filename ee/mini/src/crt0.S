// Part of neutrino: the EE side without an SDK. See README.md.
// The start of a program: clear .bss, the main thread (SetupThread gives
// argc and argv), the working directory, main, exit.
//
// Started as a thread by a loader that stays (ps2link, mxlink), a0 is its
// block {pid, argc, argv[16], payload}: its arguments are used when
// SetupThread gives none, as ps2sdk's crt0 does.
//
// However it was started, the program first starts again by ExecPS2, once,
// with its arguments: that ends the threads and the interrupt and DMA
// handlers of a loader that stayed (in memory the program takes over), and
// the BIOS sets the EE's units up again (GS, INTC, timers, DMAC, VIF, GIF,
// VU, IPU). The IOP and its devices stay as the loader left them, so the
// files next to the program can still be read. .data is not loaded again:
// _restarted says that it was done.
    .set    noreorder
    .set    noat
    .section .text._start, "ax", @progbits
    .globl  _start
    .type   _start, @function
_start:
    // .bss, in quadwords
    la      $2, _fbss
    la      $3, _end
1:  sltu    $1, $2, $3
    beqz    $1, 2f
    nop
    sq      $0, 0($2)
    b       1b
    addiu   $2, $2, 16
2:
    la      $2, _loader_args
    sw      $4, 0($2)
    // SetupThread(gp, stack, stack size, args, root): returns the stack
    la      $4, _gp
    la      $5, _stack
    la      $6, _stack_size
    la      $7, _args
    la      $8, _root
    move    $28, $4
    li      $3, 60
    syscall
    move    $29, $2

    // FlushCache(0)
    move    $4, $0
    li      $3, 100
    syscall

    // SetupThread's arguments, or the loader's
    la      $2, _loader_args
    lw      $17, 0($2)
    la      $16, _args
    lw      $2, 0($16)
    bnez    $2, 3f
    nop
    beqz    $17, 3f
    nop
    lw      $2, 4($17)
    bnez    $2, 3f
    addiu   $16, $17, 4
    la      $16, _args
3:
    // once: ExecPS2(_start, _gp, argc, argv)
    la      $2, _restarted
    lw      $1, 0($2)
    bnez    $1, 4f
    li      $1, 1
    sw      $1, 0($2)
    move    $4, $0         // FlushCache(0): _restarted to memory
    li      $3, 100
    syscall
    la      $4, _start
    la      $5, _gp
    lw      $6, 0($16)
    addiu   $7, $16, 4
    li      $3, 7          // ExecPS2
    syscall
4:
    // __init_cwd(argc, argv)
    lw      $4, 0($16)
    jal     __init_cwd
    addiu   $5, $16, 4

    ei
    lw      $4, 0($16)
    jal     main
    addiu   $5, $16, 4
    j       exit
    move    $4, $2
    .size   _start, . - _start

    .type   _root, @function
_root:
    li      $3, 35         // ExitThread
    syscall
    .size   _root, . - _root

    .data
    .p2align 2
_restarted:
    .word   0

    .bss
    .p2align 2
_loader_args:
    .space  4
    .p2align 6
_args:
    .space  4 + 16 * 4 + 256   // argc, 16 arguments, 256 bytes of them
