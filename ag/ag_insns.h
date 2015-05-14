#ifndef AG_INSNS_H
#define AG_INSNS_H

#define AG_NEON_VR3_ADDI 0x02000800
#define AG_NEON_VR3_ADDF 0x02000d00


#define AG_FOR_EACH_VR3_IF(F)                      \
    F(vadd, AG_NEON_VR3_ADDI, AG_NEON_VR3_ADDF)    \


#define AG_NEON_VR3_VPMAXS 0x02000a00
#define AG_NEON_VR3_VPMAXU 0x03000a00
#define AG_NEON_VR3_VPMAXF 0x03000f00

#define AG_NEON_VR3_VPMINS 0x02000a10
#define AG_NEON_VR3_VPMINU 0x03000a10
#define AG_NEON_VR3_VPMINF 0x03200f00

#define AG_FOR_EACH_VR3_USF(F)                       \
    F(vpmax, AG_NEON_VR3_VPMAXU, AG_NEON_VR3_VPMAXS, AG_NEON_VR3_VPMAXF) \
    F(vpmin, AG_NEON_VR3_VPMINU, AG_NEON_VR3_VPMINS, AG_NEON_VR3_VPMINF)


#define AG_NEON_VR3_VMULI 0x02000910
#define AG_NEON_VR3_VMULP 0x03000910
#define AG_NEON_VR3_VMULF 0x03000d10

#define AG_FOR_EACH_VR3_IPF(F)                                          \
    F(vmul, AG_NEON_VR3_VMULI, AG_NEON_VR3_VMULP, AG_NEON_VR3_VMULF)

#define AG_NEON_VR3_VMULLU 0x03800c00
#define AG_NEON_VR3_VMULLS 0x02800c00
#define AG_NEON_VR3_VMULLI 0x03800e00
#define AG_NEON_VR3_VMULLP 0x02800e00

#define AG_FOR_EACH_VR3_USIP(F)                                         \
    F(vmull, AG_NEON_VR3_VMULLU, AG_NEON_VR3_VMULLS, AG_NEON_VR3_VMULLI, AG_NEON_VR3_VMULLP)

#define AG_VLD1_1 0xf4a00000
#define AG_VLD2_1 0xf4a00100
#define AG_VLD3_1 0xf4a00200
#define AG_VLD4_1 0xf4a00300

#define AG_VST1_1 0xf4800000
#define AG_VST2_1 0xf4800100
#define AG_VST3_1 0xf4800200
#define AG_VST4_1 0xf4800300


#define AG_FOR_EACH_VLDST_NELEM(F, type, size_bits)      \
    F(vld, 1, type, AG_VLD1_1, size_bits)                           \
    F(vld, 2, type, AG_VLD2_1, size_bits)                            \
    F(vld, 3, type, AG_VLD3_1, size_bits)                            \
    F(vld, 4, type, AG_VLD4_1, size_bits)                            \
    F(vst, 1, type, AG_VST1_1, size_bits)                           \
    F(vst, 2, type, AG_VST2_1, size_bits)                            \
    F(vst, 3, type, AG_VST3_1, size_bits)                            \
    F(vst, 4, type, AG_VST4_1, size_bits)

#define AG_FOR_EACH_VLDST(F)                                         \
    AG_FOR_EACH_VLDST_NELEM(F, 8, 0)                                     \
    AG_FOR_EACH_VLDST_NELEM(F, 16, 1)                                     \
    AG_FOR_EACH_VLDST_NELEM(F, 32, 2)

#define AG_LDR_REG 0x06100000
#define AG_STR_REG 0x06000000

#define AG_LDR_IMM 0x05100000
#define AG_STR_IMM 0x05000000

#endif
