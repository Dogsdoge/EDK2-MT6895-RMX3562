//
// MT6895 PMIC wrapper stub.
//
// The Xiaomi pearl (MT6895) uses the MT6363/MT6368 PMICs on the SPMI bus
// (pmif @ 0x1c804000). The legacy PWRAP block does not exist on this
// platform and SPMI support is not implemented in this firmware yet, so
// PmicWrapperDxe is not included in the pearl FDF. This stub exists only to
// satisfy the DSC build; it is never instantiated.
//

#include <Protocol/MtkPmicWrapper.h>

MTK_PWRAP_REG PwrapRegs[PWRAP_REG_MAX] = {
  0x0,   // PWRAP_INIT_DONE2
  0x880, // PWRAP_WACS2_CMD
  0x8a4, // PWRAP_WACS2_VLDCLR
  0x8a8, // PWRAP_WACS2_RDATA
  0x884, // PWRAP_SWINF_2_WDATA_31_0
  0x894, // PWRAP_SWINF_2_RDATA_31_0
};

MTK_PWRAP_PLATFORM_INFO PlatformInfo = {
  0x0,   // BaseAddr: unused, PmicWrapperDxe is not shipped on pearl
  PwrapRegs,
  FALSE  // ArbCap
};
