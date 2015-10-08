#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <string.h>
#include <stdio.h>
#include <linux/perf_event.h>
#include <stdlib.h>
#include "ag/ag_gen.h"

static int
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags )
{
    int ret;

    ret = syscall( __NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags );
    return ret;
}

static int perf_fd;

static uint64_t
read_cycle(void)
{
    uint64_t ret;
    ssize_t rsz = read(perf_fd, &ret, 8);
    if (rsz != 8) {
        perror("read perf");
        exit(1);
    }

    return ret;
}

char __attribute__((aligned(64))) zero_mem[4096*8];

enum lt_op {
    LT_LATENCY,
    LT_THROUGHPUT,
    LT_THROUGHPUT_RENAME
};

enum operand_type {
    OT_INT,
    OT_FP32,
    OT_FP64,
    OT_F32x1,
    OT_F32x2,
    OT_F32x4
};

enum regtype {
    REG_GEN,
    REG_NEON_64b,
    REG_NEON_128b
};

static const char *regtype_name_table[] = {
    "generic",
    "neon64",
    "neon128"
};

#define ZEROMEM_PTR_REG 11

template <typename F>
static void
gen(struct ag_Emitter *e,
    enum regtype rt, F f,
    int num_loop, int num_insn, enum lt_op o, enum operand_type ot)
{
    /* A32 regisuter usage
     * http://infocenter.arm.com/help/topic/com.arm.doc.ihi0042e/IHI0042E_aapcs.pdf
     *
     * r0, r1, r2, r3                 argument, scratch (caller save)
     * r4, r5, r6, r7, r8, r10, r11   variable (callee save)
     * r9                             platform (callee??)
     * r13                            stack    (callee save)
     * r14                            lr       (callee save)
     */

    /* r0-r9 : operand register
     * r10 : loop counter
     * r11 : ptr to zero mem
     */

    /* save r4-r11 */
    /*                            109876543210 */
    ag_emit_push(e, AG_COND_AL, 0b111111110000);
    ag_emit_movldr_imm(e, AG_COND_AL, 10, num_loop);
    ag_emit_movldr_imm(e, AG_COND_AL, ZEROMEM_PTR_REG, (uintptr_t)&zero_mem);

    for (int i=0; i<10; i++) {
        ag_emit_movldr_imm(e, AG_COND_AL, i, 0);
    }

    for (int i=0; i<16; i++) {
        ag_emit_vdup32(e, AG_COND_AL, 1, i*2, 0);
    }


    ag_label_id_t loop_head = ag_emit_new_label(e, NULL);

    switch (o) {
    case LT_LATENCY:
        for (int ii=0; ii<num_insn; ii++) {
            f(e, 0, 0);
        }
        break;

    case LT_THROUGHPUT:
        for (int ii=0; ii<num_insn/8; ii++) {
            f(e, 0, 1);
            f(e, 0, 2);
            f(e, 0, 3);
            f(e, 0, 4);

            f(e, 0, 5);
            f(e, 0, 6);
            f(e, 0, 7);
            f(e, 0, 8);
        }
        break;

    case LT_THROUGHPUT_RENAME:
        for (int ii=0; ii<num_insn/8; ii++) {
            f(e, 0, 0);
            f(e, 1, 1);
            f(e, 2, 2);
            f(e, 3, 3);

            f(e, 4, 4);
            f(e, 5, 5);
            f(e, 6, 6);
            f(e, 7, 7);
        }
        break;
    }


    ag_emit_sub_imm(e, AG_COND_AL, 1, 10, 10, 1);
    ag_emit_b(e, AG_COND_NE, loop_head);

    ag_emit_pop(e, AG_COND_AL, 0b111111110000);

    ag_emit_bx(e, AG_COND_AL, AG_LR);
}



template <typename F>
void
lt(const char *name,
   const char *on,
   enum regtype rt, F f,
   int num_loop,
   int num_insn,
   enum lt_op o,
   enum operand_type ot)
{
    struct ag_Emitter e;
    ag_emitter_init(&e);

    gen(&e, rt, f, num_loop, num_insn, o, ot);

    void *code;
    size_t code_size;

    ag_alloc_code(&code, &code_size, &e);

#ifdef EMIT_ONLY
    FILE *fp = fopen("test.bin", "wb");
    fwrite(code, 1, code_size, fp);
    fclose(fp);
#else
    typedef void (*func_t)(void);

#ifdef __ANDROID__
    FILE *fp = fopen("/sdcard/test.bin", "wb");
    fwrite(code, 1, code_size, fp);
    fclose(fp);
#endif

    func_t exec = (func_t)code;
    exec();

    uint64_t tb = read_cycle();
    exec();
    uint64_t te = read_cycle();

    printf("%8s : %50s : %15s : CPI=%8.2f, IPC=%8.2f\n",
           regtype_name_table[(int)rt],
           name, on,
           (te-tb)/(double)(num_insn * num_loop), 
           (num_insn * num_loop)/(double)(te-tb));
#endif


    ag_emitter_fini(&e);
}


#define NUM_LOOP (16384*8)

template <typename F>
void
run(const char *name, enum regtype rt, F f, enum operand_type ot, int num_insn)
{
    lt(name, "latency", rt, f, NUM_LOOP, num_insn, LT_LATENCY, ot);
    lt(name, "throughput", rt, f, NUM_LOOP, num_insn, LT_THROUGHPUT, ot);
    lt(name, "rename", rt, f, NUM_LOOP, num_insn, LT_THROUGHPUT_RENAME, ot);
}

template <typename F>
void
run_latency(const char *name, enum regtype rt, F f, enum operand_type ot, int num_insn)
{
    lt(name, "latency", rt, f, NUM_LOOP, num_insn, LT_LATENCY, ot);
}

template <typename F>
void
run_throughput(const char *name, enum regtype rt, F f, enum operand_type ot, int num_insn)
{
    lt(name, "throughput", rt, f, NUM_LOOP, num_insn, LT_THROUGHPUT_RENAME, ot);
    lt(name, "rename", rt, f, NUM_LOOP, num_insn, LT_THROUGHPUT_RENAME, ot);
}

#define GEN(rt, name, expr, ot)                                          \
    run(                                                                \
    name,                                                               \
    rt,                                                                 \
    [](struct ag_Emitter *e, int dst, int src){expr;},                  \
    ot, num_insn);


#define GEN_latency(rt, name, expr, ot)                               \
    run_latency(                                                        \
        name,                                                           \
        rt,                                                             \
        [](struct ag_Emitter *e, int dst, int src){expr;},              \
        ot, num_insn);

#define GEN_throughput(rt, name, expr, ot)                               \
    run_throughput(                                                     \
        name,                                                           \
        rt,                                                             \
        [](struct ag_Emitter *e, int dst, int src){expr;},              \
        ot, num_insn);

int
main()
{
    struct perf_event_attr attr;

    memset(&attr, 0, sizeof(attr));

    attr.type = PERF_TYPE_HARDWARE;
    attr.size = sizeof(attr);
    attr.config = PERF_COUNT_HW_CPU_CYCLES;

    perf_fd = perf_event_open(&attr, 0, -1, -1, 0);
    if (perf_fd == -1) {
        perror("perf_event_open");
        exit(1);
    }

    int num_insn = 16;
    while (num_insn <= 256) {
        printf("== num_insn = %d ==\n", num_insn);

        GEN(REG_GEN, "add rd, rm, rn",
            ag_emit_add_reg(e, AG_COND_AL, 0, dst, src, src, 0),
            OT_INT);

        GEN(REG_GEN, "adds rd, rm, rn",
            ag_emit_add_reg(e, AG_COND_AL, 1, dst, src, src, 0),
            OT_INT);

        GEN(REG_GEN, "add rd, rm, rn, lsl #4",
            ag_emit_add_reg(e, AG_COND_AL, 1, dst, src, src, AG_LSL_AM(4)),
            OT_INT);

        GEN(REG_GEN, "add rd, rm, imm",
            ag_emit_add_imm(e, AG_COND_AL, 0, dst, src, 100),
            OT_INT);

        GEN(REG_GEN, "add rd, rm, pc",
            ag_emit_add_reg(e, AG_COND_AL, 0, dst, src, AG_PC, AG_LSL_AM(4)),
            OT_INT);

        GEN_latency(REG_GEN, "add pc, pc, 0",
                    ag_emit_add_imm(e, AG_COND_AL, 0, AG_PC, AG_PC, 0),
                    OT_INT);

        GEN(REG_GEN, "orr rd, rm, rn",
            ag_emit_orr_reg(e, AG_COND_AL, 0, dst, src, src, 0),
            OT_INT);

        GEN(REG_GEN, "eor rd, rm, rn",
            ag_emit_orr_reg(e, AG_COND_AL, 0, dst, src, src, 0),
            OT_INT);

        GEN(REG_GEN, "mul rd, rm, rs",
            ag_emit_mul(e, AG_COND_AL, 0, dst, src, src),
            OT_INT);

        GEN(REG_GEN, "mla rd, rm, rs, rn",
            ag_emit_mla(e, AG_COND_AL, 0, dst, src, src, src),
            OT_INT);

        GEN(REG_GEN, "ldr rt, [rn, rm]",
            ag_emit_ldr_reg(e, AG_COND_AL, dst, ZEROMEM_PTR_REG, src, 0, 1, AG_OFFSET_ADDR),
            OT_INT);

        GEN(REG_GEN, "ldr rt, [rn, rm, lsl #4]",
            ag_emit_ldr_reg(e, AG_COND_AL, dst, ZEROMEM_PTR_REG, src, AG_LSL_AM(4), 1, AG_OFFSET_ADDR),
            OT_INT);

        GEN_throughput(REG_GEN, "ldm rt, {r0}",
                       ag_emit_ldmia(e, AG_COND_AL, 0, ZEROMEM_PTR_REG, 0b1),
                       OT_INT);

        GEN_throughput(REG_GEN, "ldm rt, {r0-r3}",
                       ag_emit_ldmia(e, AG_COND_AL, 0, ZEROMEM_PTR_REG, 0b1111),
                       OT_INT);

        GEN_throughput(REG_GEN, "ldm rt, {r0-r7}",
                       ag_emit_ldmia(e, AG_COND_AL, 0, ZEROMEM_PTR_REG, 0b11111111),
                       OT_INT);

        GEN_throughput(REG_GEN, "ldrex rd, [rn]",
                       ag_emit_ldrex(e, AG_COND_AL, 0, ZEROMEM_PTR_REG),
                       OT_INT);
        GEN_throughput(REG_GEN, "strex rd, rm, [rn]",
                       ag_emit_strex(e, AG_COND_AL, 0, 1, ZEROMEM_PTR_REG),
                       OT_INT);

        GEN_throughput(REG_GEN, "ldrex r0, [rn]; strex rd, r0, [rn] ",
                       ag_emit_ldrex(e, AG_COND_AL, 0, ZEROMEM_PTR_REG);
                       ag_emit_strex(e, AG_COND_AL, 1, 0, ZEROMEM_PTR_REG),
                       OT_INT);

        GEN_throughput(REG_GEN, "str rt, [rn, #0]",
                       ag_emit_str_imm(e, AG_COND_AL, dst, ZEROMEM_PTR_REG, 0, 0),
                       OT_INT);


        GEN_latency(REG_GEN, "{str->ldr}->...",
                    ag_emit_str_imm(e, AG_COND_AL, dst, ZEROMEM_PTR_REG, 0, 0);
                    ag_emit_ldr_reg(e, AG_COND_AL, dst, ZEROMEM_PTR_REG, dst, 0, 1, AG_OFFSET_ADDR),
                    OT_INT);

        GEN_latency(REG_GEN, "{strb->ldr}->...",
                    ag_emit_strb_imm(e, AG_COND_AL, dst, ZEROMEM_PTR_REG, 1, 0);
                    ag_emit_ldr_reg(e, AG_COND_AL, dst, ZEROMEM_PTR_REG, dst, 0, 1, AG_OFFSET_ADDR),
                    OT_INT);

        GEN(REG_NEON_64b, "vadd.f32 d, d, d",
            ag_emit_vadd_f32(e, 0, dst, src, src),
            OT_F32x2);
        GEN(REG_NEON_128b, "vadd.f32 q, q, q",
            ag_emit_vadd_f32(e, 1, dst, src, src),
            OT_F32x4);

        GEN(REG_NEON_64b, "vmul.f32 d, d, d",
            ag_emit_vmul_f32(e, 0, dst, src, src),
            OT_F32x2);
        GEN(REG_NEON_128b, "vmul.f32 q, q, q",
            ag_emit_vmul_f32(e, 1, dst, src, src),
            OT_F32x4);

        GEN(REG_NEON_64b, "vmul.f32 d, d, d",
            ag_emit_vmul_f32(e, 0, dst, src, src),
            OT_F32x2);
        GEN(REG_NEON_128b, "vmul.f32 q, q, q",
            ag_emit_vmul_f32(e, 1, dst, src, src),
            OT_F32x4);

        GEN(REG_NEON_64b, "vmla.f32 d, d, d",
            ag_emit_vmla_f32(e, 0, dst, src, src),
            OT_F32x2);
        GEN(REG_NEON_128b, "vmla.f32 q, q, q",
            ag_emit_vmla_f32(e, 1, dst, src, src),
            OT_F32x4);

        GEN_throughput(REG_NEON_64b, "vld1.32 d, [rn]",
                       ag_emit_vld1_32(e, dst, ZEROMEM_PTR_REG, 15, 0),
                       OT_F32x1);
        GEN_throughput(REG_NEON_64b, "vld2.32 d, [rn]",
                       ag_emit_vld2_32(e, dst, ZEROMEM_PTR_REG, 15, 0),
                       OT_F32x2);
        GEN_throughput(REG_NEON_128b, "vld4.32 q, [rn]",
                       ag_emit_vld4_32(e, dst, ZEROMEM_PTR_REG, 15, 0),
                       OT_F32x4);

        GEN_throughput(REG_NEON_64b, "vst1.32 d, [rn]",
                       ag_emit_vst1_32(e, dst, ZEROMEM_PTR_REG, 15, 0),
                       OT_F32x1);
        GEN_throughput(REG_NEON_64b, "vst2.32 d, [rn]",
                       ag_emit_vst2_32(e, dst, ZEROMEM_PTR_REG, 15, 0),
                       OT_F32x2);
        GEN_throughput(REG_NEON_128b, "vst4.32 q, [rn]",
                       ag_emit_vst4_32(e, dst, ZEROMEM_PTR_REG, 15, 0),
                       OT_F32x4);

        GEN_throughput(REG_NEON_64b, "vcvt.f32.s32 d, d",
                       ag_emit_vcvt_f32_s32(e, 0, dst, src),
                       OT_F32x2);
        GEN_throughput(REG_NEON_128b, "vcvt.f32.s32 q, q",
                       ag_emit_vcvt_f32_s32(e, 1, dst*2, src*2),
                       OT_F32x4);

        GEN_throughput(REG_NEON_64b, "vcvt.s32.f32 d, d",
                       ag_emit_vcvt_s32_f32(e, 0, dst, src),
                       OT_F32x2);
        GEN_throughput(REG_NEON_128b, "vcvt.s32.f32 q, q",
                       ag_emit_vcvt_s32_f32(e, 1, dst*2, src*2),
                       OT_F32x4);

        num_insn *= 2;
    }
}

