# mini

The little that neutrino's EE programs need of an SDK, on the PS2's BIOS: system call stubs, cache
operations, the SIF command and RPC client, and on top of it LOADFILE, the IOP reset and heap, the
ROM's FILEIO and fileXio clients, and the LoadModuleBuffer patch. Built with LLVM,
without ps2sdk.

The headers and functions are called as in ps2sdk, so code for ps2sdk moves here unchanged. Only what
neutrino uses is there. The protocols and the structures are those of the IOP's modules in the ROM; they,
and the LoadModuleBuffer patch, are as in ps2sdk (ps2dev, http://www.ps2dev.org, Academic Free License
version 2.0).

- `include/libc`: the headers of a C library, for programs without one (`ee_core`).
- The loader uses picolibc: `src/posix.c` gives it files, paths, the working directory and stdout on
  FILEIO and fileXio, as ps2sdk's libcglue does; `src/crt0.S` and `ld/program.ld` start it.
- `src/crt0.S` takes the arguments from SetupThread, or from the block a loader that starts the
  program as a thread passes (ps2link, mxlink), as ps2sdk's crt0 does. It then starts the program
  again by ExecPS2, once: the threads and handlers of a loader that stayed are gone before the program
  takes over the memory below 1 MiB, and the BIOS sets the EE's units up again. The IOP keeps the
  loader's modules, so the program still reads the files next to it.
- There is no RPC server: the EE is a client, and answers the IOP's reads of EE memory (RPC_RDATA).
