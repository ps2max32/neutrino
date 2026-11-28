// mmcefhi includes the FHI header by its old name, and defines fhi_size with
// ps2sdk's u32 (unsigned long), fhi.h declares it with uint32_t (unsigned int):
// the same in the ABI, so fhi.h's declaration is left out here.
#define fhi_size fhi_size_fhi_h
#include "fhi_fileid_config.h"
#undef fhi_size
