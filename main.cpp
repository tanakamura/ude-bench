#define AG_USE_SHORT_REG

#include "ag/ag_gen.h"
#include <stdio.h>

int
main()
{
    struct ag_Emitter e;
    ag_emitter_init(&e);

    ag_label_id_t l1 = ag_emit_new_label(&e, "test");
    ag_label_id_t l2 = ag_alloc_label(&e, "test2");

    for (int i=0; i<1; i++) {
        ag_emit_bl(&e, AG_COND_AL, l1);
        ag_emit_bl(&e, AG_COND_AL, l2);
        ag_emit_bx(&e, AG_COND_AL, LR);
        ag_emit_add_reg(&e, AG_COND_GT, 1, R0, R1, AG_LSL_REG(1), R2);
        ag_emit_and_reg(&e, AG_COND_GT, 1, R0, R1, AG_LSL_REG(1), R2);

        ag_emit_add_imm(&e, AG_COND_GT, 1, R0, R1, 100);
        ag_emit_and_imm(&e, AG_COND_GT, 1, R0, R1, 100);

        ag_emit_vadd_i32(&e, 0, 0, 1, 2);
        ag_emit_vadd_i8(&e, 0, 31, 31, 31);
        ag_emit_vadd_i32(&e, 1, 0, 1, 2);
        ag_emit_vadd_f64(&e, 0, 0, 1, 2);

        ag_emit_vpmin_s32(&e, 0, 0, 1, 2);
        ag_emit_vpmax_u32(&e, 0, 0, 1, 2);
        ag_emit_vpmax_f32(&e, 0, 0, 1, 2);
        ag_emit_vpmin_f32(&e, 0, 0, 1, 2);

        ag_emit_vmul_f32(&e, 0, 0, 1, 2);
        ag_emit_vmul_i32(&e, 0, 0, 1, 2);
        ag_emit_vmul_p32(&e, 0, 0, 1, 2);

        ag_emit_vmull_u32(&e, 0, 0, 1, 2);
        ag_emit_vmull_s32(&e, 0, 0, 1, 2);
        ag_emit_vmull_i8(&e, 0, 0, 1, 2);
        ag_emit_vmull_p8(&e, 0, 0, 1, 2);
    }

    ag_emit_label(&e, l2);

    void *code;
    size_t code_size;

    ag_alloc_code(&code, &code_size, &e);
    FILE *fp = fopen("test.bin", "wb");
    fwrite(code, 1, code_size, fp);
    fclose(fp);

    ag_emitter_fini(&e);
}
