#ifndef AG_GEN_H
#define AG_GEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "npr/varray.h"
#include "ag/ag_insns.h"


struct ag_Emitter {
    unsigned int cur;
    struct CodeBufferBlock *last;

    struct npr_varray labels;
    struct npr_varray label_refs;

    unsigned char *code;
    size_t code_size;
};

enum ag_cond {
    AG_COND_EQ=0,
    AG_COND_NE=1,
    AG_COND_CS=2,
    AG_COND_CC=3,
    AG_COND_MI=4,
    AG_COND_PL=5,
    AG_COND_VS=6,
    AG_COND_VC=7,
    AG_COND_HI=8,
    AG_COND_LS=9,
    AG_COND_GE=10,
    AG_COND_LT=11,
    AG_COND_GT=12,
    AG_COND_LE=13,
    AG_COND_AL=14,
};

#ifdef AG_USE_SHORT_REG
#define R0 0
#define R1 1
#define R2 2
#define R3 3

#define R4 4
#define R5 5
#define R6 6
#define R7 7

#define R8 8
#define R9 9
#define R10 10
#define R11 11

#define R12 12

#define SP 13
#define LR 14
#define PC 15
#endif

#define AG_SP 13
#define AG_LR 14
#define AG_PC 15

#define AG_SHIFT_LOG_LEFT 0
#define AG_SHIFT_LOG_RIGHT 1

#define AG_SHIFT_ARITH_LEFT 2
#define AG_SHIFT_ROTATE_RIGHT 3

#define AG_SHIFT_AMOUNT(am, type) (((am)<<3) | ((type)<<1))

#define AG_LSL_AM(am) AG_SHIFT_AMOUNT(am, AG_SHIFT_LOG_LEFT)
#define AG_LSR_AM(am) AG_SHIFT_AMOUNT(am, AG_SHIFT_LOG_RIGHT)
#define AG_ASR_AM(am) AG_SHIFT_AMOUNT(am, AG_SHIFT_ARITH_RIGHT)
#define AG_ROR_AM(am) AG_SHIFT_AMOUNT(am, AG_ROTATE_RIGHT)

#define AG_SHIFT_REG(reg, type) (((reg)<<4) | ((type)<<1) | 1)

#define AG_LSL_REG(r) AG_SHIFT_REG(r, AG_SHIFT_LOG_LEFT)
#define AG_LSR_REG(r) AG_SHIFT_REG(r, AG_SHIFT_LOG_RIGHT)
#define AG_ASR_REG(r) AG_SHIFT_REG(r, AG_SHIFT_ARITH_RIGHT)
#define AG_ROR_REG(r) AG_SHIFT_REG(r, AG_ROTATE_RIGHT)

void ag_emitter_init(struct ag_Emitter *e);
void ag_emitter_fini(struct ag_Emitter *e);

typedef unsigned int ag_label_id_t;

void ag_emit_label(struct ag_Emitter *e, ag_label_id_t label);
ag_label_id_t ag_alloc_label(struct ag_Emitter *e, const char *name /* optional, maybe NULL */);

/* alloc & emit label to cursor */
ag_label_id_t ag_emit_new_label(struct ag_Emitter *e, const char *name /* optional, maybe NULL */);

void ag_emit_bx(struct ag_Emitter *e, enum ag_cond cc, int reg);

void ag_emit_b0(struct ag_Emitter *e, enum ag_cond cc, int l, ag_label_id_t dst);
void ag_emit_b(struct ag_Emitter *e, enum ag_cond cc, ag_label_id_t dst);
void ag_emit_bl(struct ag_Emitter *e, enum ag_cond cc, ag_label_id_t dst);

enum ag_data_process_opcode {
    AG_AND = 0,
    AG_EOR = 1,
    AG_SUB = 2,
    AG_RSB = 3,
    AG_ADD = 4,
    AG_ADC = 5,
    AG_SBC = 6,
    AG_RSC = 7,

    AG_TST = 8,
    AG_TEQ = 9,
    AG_CMP = 10,
    AG_CMN = 11,
    AG_ORR = 12,
    AG_MOV = 13,
    AG_BIC = 14,
    AG_MVN = 15
};

void ag_emit_data_process_reg(struct ag_Emitter *e, enum ag_cond cc, enum ag_data_process_opcode opc,
                              int s, int rd, int rn, int rm, int shift);

void ag_emit_and_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rm, int rn, int shift);
void ag_emit_eor_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rm, int rn, int shift);
void ag_emit_sub_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rm, int rn, int shift);
void ag_emit_rsb_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rm, int rn, int shift);
void ag_emit_add_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rm, int rn, int shift);
void ag_emit_adc_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rm, int rn, int shift);
void ag_emit_sbc_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rm, int rn, int shift);
void ag_emit_rsc_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rm, int rn, int shift);

void ag_emit_tst_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rm, int rn, int shift);
void ag_emit_teq_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rm, int rn, int shift);
void ag_emit_cmp_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rm, int rn, int shift);
void ag_emit_cmn_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rm, int rn, int shift);
void ag_emit_orr_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rm, int rn, int shift);
void ag_emit_mov_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rm, int rn, int shift);
void ag_emit_bic_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rm, int rn, int shift);
void ag_emit_mvn_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rm, int rn, int shift);


/* return negative if imm is out of range */
int ag_emit_data_process_imm(struct ag_Emitter *e, enum ag_cond cc, enum ag_data_process_opcode opc,
                             int s, int rd, int rn, int32_t imm);
int ag_emit_and_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm);
int ag_emit_eor_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm);
int ag_emit_sub_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm);
int ag_emit_rsb_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm);
int ag_emit_add_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm);
int ag_emit_adc_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm);
int ag_emit_sbc_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm);
int ag_emit_rsc_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm);

int ag_emit_tst_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm);
int ag_emit_teq_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm);
int ag_emit_cmp_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm);
int ag_emit_cmn_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm);
int ag_emit_orr_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm);
int ag_emit_mov_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm);
int ag_emit_bic_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm);
int ag_emit_mvn_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm);


void ag_emit_vr3(struct ag_Emitter *E, int opc, int q, int size, int vd, int vn, int vm);

#define AG_VR3_IF_GEN_PROTO(name, opci, opcf)                           \
    void ag_emit_##name##_i32(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_i16(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_i8(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_f32(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_f64(struct ag_Emitter *E, int q, int vd, int vn, int vm);

AG_FOR_EACH_VR3_IF(AG_VR3_IF_GEN_PROTO);


#define AG_VR3_USF_GEN_PROTO(name, opcu, opcs, opcf)                    \
    void ag_emit_##name##_s32(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_s16(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_s8(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_u32(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_u16(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_u8(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_f32(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_f64(struct ag_Emitter *E, int q, int vd, int vn, int vm);

AG_FOR_EACH_VR3_USF(AG_VR3_USF_GEN_PROTO);

#define AG_VR3_IPF_GEN_PROTO(name, opci, opcp, opcf)                    \
    void ag_emit_##name##_i32(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_i16(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_i8(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_p32(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_p16(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_p8(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_f32(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_f64(struct ag_Emitter *E, int q, int vd, int vn, int vm);

AG_FOR_EACH_VR3_IPF(AG_VR3_IPF_GEN_PROTO);


#define AG_VR3_USIP_GEN_PROTO(name, opcu, opcs, opci, opcp)             \
    void ag_emit_##name##_u32(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_u16(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_u8(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_s32(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_s16(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_s8(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_i32(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_i16(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_i8(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_p32(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_p16(struct ag_Emitter *E, int q, int vd, int vn, int vm); \
    void ag_emit_##name##_p8(struct ag_Emitter *E, int q, int vd, int vn, int vm); \

AG_FOR_EACH_VR3_USIP(AG_VR3_USIP_GEN_PROTO);

void ag_alloc_code(void **ret, size_t *ret_size,
                   struct ag_Emitter *e); /* do not call twice per ag_Emitter */


#ifdef __cplusplus
}
#endif


#endif