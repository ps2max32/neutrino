# mini

The little that neutrino's EE programs need of an SDK, on the PS2's BIOS: system call stubs, cache
operations, the SIF command and RPC client, and a few clients and patches on top of it. Built with LLVM,
without ps2sdk.

The headers and functions are called as in ps2sdk, so code for ps2sdk moves here unchanged. Only what
neutrino uses is there. The protocols and the structures are those of the IOP's modules in the ROM; they,
and the LoadModuleBuffer patch, are as in ps2sdk (ps2dev, http://www.ps2dev.org, Academic Free License
version 2.0).

- `include/libc`: the headers of a C library, for programs without one (`ee_core`).
- There is no RPC server: the EE is a client, and answers the IOP's reads of EE memory (RPC_RDATA).
