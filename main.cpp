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
    LT_THROUGHPUT_KILLDEP
};

enum operand_type {
    OT_INT,
    OT_FP32,
    OT_FP64
};

enum regtype {
    REG_GEN,
    REG_NEON_64b,
    REG_NEON_128b
};

static const char *regtype_name_table[] = {
    "gen",
    "neon64",
    "neon128"
};


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

    /* r0-r7 : operand register
     * r8 : loop counter
     * r9 : ptr to zero mem
     */

    ag_emit_push(e, AG_COND_AL, 0b1111110000);
    ag_emit_movldr_imm(e, AG_COND_AL, 8, num_loop - 1);

    ag_label_id_t loop_head = ag_emit_new_label(e, NULL);

    ag_emit_sub_imm(e, AG_COND_AL, 1, 8, 8, 1);
    ag_emit_cmp_reg(e, AG_COND_AL, 8, 8, 0);
    ag_emit_b(e, AG_COND_EQ, loop_head);

    ag_emit_pop(e, AG_COND_AL, 0b1111110000);

    ag_emit_bx(e, AG_COND_AL, AG_LR);
}

#if 0
template <enum regtype RegType,
          typename F>
struct Gen
{
    Gen(struct ag_Emitter *e, F f, int num_loop, int num_insn, enum lt_op o, enum operand_type ot) {
        ag_emit_bx(e, AG_COND_AL, AG_LR);
        RegMap<RegType> rm;

        push(rbp);
        mov(rbp, rsp);
        and_(rsp, -(Xbyak::sint64)32);
        sub(rsp, 32 * 9);

        rm.save(this, rm.v8,  -32*8, ot);
        rm.save(this, rm.v9,  -32*7, ot);
        rm.save(this, rm.v10, -32*6, ot);
        rm.save(this, rm.v11, -32*5, ot);
        rm.save(this, rm.v12, -32*4, ot);
        rm.save(this, rm.v13, -32*3, ot);
        rm.save(this, rm.v14, -32*2, ot);
        rm.save(this, rm.v15, -32*1, ot);

        rm.killdep(this, rm.v8, ot);
        rm.killdep(this, rm.v9, ot);
        rm.killdep(this, rm.v10, ot);
        rm.killdep(this, rm.v11, ot);
        rm.killdep(this, rm.v12, ot);
        rm.killdep(this, rm.v13, ot);
        rm.killdep(this, rm.v14, ot);
        rm.killdep(this, rm.v15, ot);

        mov(rcx, num_loop);
        mov(rdx, (intptr_t)zero_mem);
        mov(ptr[rsp], rdi);
        xor_(rdi, rdi);

        L("@@");

        switch (o) {
        case LT_LATENCY:
            for (int ii=0; ii<num_insn; ii++) {
                f(this, rm.v8, rm.v8);
            }
            break;

        case LT_THROUGHPUT:
            for (int ii=0; ii<num_insn/8; ii++) {
                f(this, rm.v8, rm.v8);
                f(this, rm.v9, rm.v9);
                f(this, rm.v10, rm.v10);
                f(this, rm.v11, rm.v11);
                f(this, rm.v12, rm.v12);
                f(this, rm.v13, rm.v13);
                f(this, rm.v14, rm.v14);
                f(this, rm.v15, rm.v15);
            }
            break;

        case LT_THROUGHPUT_KILLDEP:
            for (int ii=0; ii<num_insn/8; ii++) {
                f(this, rm.v8, rm.v8);
                f(this, rm.v9, rm.v9);
                f(this, rm.v10, rm.v10);
                f(this, rm.v11, rm.v11);
                f(this, rm.v12, rm.v12);
                f(this, rm.v13, rm.v13);
                f(this, rm.v14, rm.v14);
                f(this, rm.v15, rm.v15);
            }

            rm.killdep(this, rm.v8, ot);
            rm.killdep(this, rm.v9, ot);
            rm.killdep(this, rm.v10, ot);
            rm.killdep(this, rm.v11, ot);
            rm.killdep(this, rm.v12, ot);
            rm.killdep(this, rm.v13, ot);
            rm.killdep(this, rm.v14, ot);
            rm.killdep(this, rm.v15, ot);
            break;
        }

        dec(rcx);
        jnz("@b");

        mov(rdi, ptr[rsp]);
        rm.restore(this, rm.v8,  -32*8, ot);
        rm.restore(this, rm.v9,  -32*7, ot);
        rm.restore(this, rm.v10, -32*6, ot);
        rm.restore(this, rm.v11, -32*5, ot);
        rm.restore(this, rm.v12, -32*4, ot);
        rm.restore(this, rm.v13, -32*3, ot);
        rm.restore(this, rm.v14, -32*2, ot);
        rm.restore(this, rm.v15, -32*1, ot);

        mov(rsp, rbp);
        pop(rbp);
        ret();

        /*
         * latency:
         *
         *       mov rcx, count
         * loop:
         *       op reg, reg
         *       op reg, reg
         *       ...
         *       op reg, reg
         *       dec rcx
         *       jne loop
         *
         */

        /*
         * throughput
         *
         *       mov rcx, count
         * loop:
         *       op reg8, reg8
         *       op reg9, reg9
         *       ...
         *       op reg15, reg15
         *       op reg8, reg8
         *       op reg9, reg9
         *       ...
         *       op reg15, reg15
         *       ...
         *       if kill_dep {
         *       xor r8
         *       xor r9
         *       ...
         *       xor r15
         *       }
         *       dec rcx
         *       jne loop
         *
         */

    }

};
#endif





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
    typedef void (*func_t)(void);

    void *code;
    size_t code_size;

    ag_alloc_code(&code, &code_size, &e);

#ifdef EMIT_ONLY
    FILE *fp = fopen("test.bin", "wb");
    fwrite(code, 1, code_size, fp);
    fclose(fp);
#else
    FILE *fp = fopen("/sdcard/test.bin", "wb");
    fwrite(code, 1, code_size, fp);
    fclose(fp);


    func_t exec = (func_t)code;
    exec();

    uint64_t tb = read_cycle();
    exec();
    uint64_t te = read_cycle();

    printf("%8s:%10s:%10s: CPI=%8.2f, IPC=%8.2f\n",
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
run(const char *name, enum regtype rt, F f, bool kill_dep, enum operand_type ot)
{
    lt(name, "latency", rt, f, NUM_LOOP, 16, LT_LATENCY, ot);
    if (kill_dep) {
        lt(name, "throughput", rt, f, NUM_LOOP, 16, LT_THROUGHPUT_KILLDEP, ot);
    } else {
        lt(name, "throughput", rt, f, NUM_LOOP, 16, LT_THROUGHPUT, ot);
    }
}

template <typename F_t, typename F_l>
void
run_latency(const char *name, enum regtype rt, F_t f_t, F_l f_l, bool kill_dep, enum operand_type ot)
{
    lt(name, "latency", rt, f_l, NUM_LOOP, 16, LT_LATENCY, ot);
    if (kill_dep) {
        lt(name, "throughput", rt, f_t, NUM_LOOP, 16, LT_THROUGHPUT_KILLDEP, ot);
    } else {
        lt(name, "throughput", rt, f_t, NUM_LOOP, 16, LT_THROUGHPUT, ot);
    }
}

#define GEN(rt, name, expr, kd, ot)                                     \
    run(                                                                \
    name,                                                               \
    rt,                                                                 \
    [](struct ag_Emitter *e, int dst, int src){expr;},                  \
    kd, ot);


#define GEN_latency(rt, name, expr_t, expr_l, kd, ot)                   \
    run_latency<rt>(                                                    \
    name,                                                               \
    [](Xbyak::CodeGenerator *g, Xbyak::rt dst, Xbyak::rt src){expr_t;}, \
    [](Xbyak::CodeGenerator *g, Xbyak::rt dst, Xbyak::rt src){expr_l;}, \
    kd, ot);

int
main()
{
    struct perf_event_attr attr;
    long long tstart, tend;

    memset(&attr, 0, sizeof(attr));

    attr.type = PERF_TYPE_HARDWARE;
    attr.size = sizeof(attr);
    attr.config = PERF_COUNT_HW_CPU_CYCLES;

    perf_fd = perf_event_open(&attr, 0, -1, -1, 0);
    if (perf_fd == -1) {
        perror("perf_event_open");
        exit(1);
    }

    read(perf_fd, &tstart, 8);
    read(perf_fd, &tend, 8);

    printf("%lld\n", tend-tstart);

    printf("== latency/throughput ==\n");
    GEN(REG_GEN, "add", , false, OT_INT);

}



#if 0

template <typename T> struct RegMap;

template <>
struct RegMap<Xbyak::Xmm>
{
    const char *name;
    Xbyak::Xmm v8, v9, v10, v11, v12, v13, v14, v15;

    RegMap()
        :name("m128"), v8(8), v9(9), v10(10), v11(11), v12(12), v13(13), v14(14), v15(15)
        {}

    void save(Xbyak::CodeGenerator *g, Xbyak::Xmm r, int off, enum operand_type ot) {
        switch (ot) {
        case OT_INT:
            g->movdqa(g->ptr [g->rsp + off], r);
            break;
        case OT_FP32:
            g->movaps(g->ptr [g->rsp + off], r);
            break;
        case OT_FP64:
            g->movapd(g->ptr [g->rsp + off], r);
            break;
        }
    }

    void restore(Xbyak::CodeGenerator *g, Xbyak::Xmm r, int off, enum operand_type ot) {
        switch (ot) {
        case OT_INT:
            g->movdqa(r, g->ptr [g->rsp + off]);
            break;
        case OT_FP32:
            g->movaps(r, g->ptr [g->rsp + off]);
            break;
        case OT_FP64:
            g->movapd(r, g->ptr [g->rsp + off]);
            break;
        }
    }

    void killdep(Xbyak::CodeGenerator *g, Xbyak::Xmm r, enum operand_type ot) {
        switch (ot) {
        case OT_INT:
            g->pxor(r, r);
            break;
        case OT_FP32:
            g->xorps(r, r);
            break;
        case OT_FP64:
            g->xorpd(r, r);
            break;
        }
    }
};

template <>
struct RegMap<Xbyak::Ymm>
{
    const char *name;
    Xbyak::Ymm v8, v9, v10, v11, v12, v13, v14, v15;

    RegMap()
        :name("m256"), v8(8), v9(9), v10(10), v11(11), v12(12), v13(13), v14(14), v15(15)
        {}

    void save(Xbyak::CodeGenerator *g, Xbyak::Ymm r, int off, enum operand_type ot) {
        switch (ot) {
        case OT_INT:
            g->vmovdqa(g->ptr [g->rsp + off], r);
            break;
        case OT_FP32:
            g->vmovaps(g->ptr [g->rsp + off], r);
            break;
        case OT_FP64:
            g->vmovapd(g->ptr [g->rsp + off], r);
            break;
        }
    }

    void restore(Xbyak::CodeGenerator *g, Xbyak::Ymm r, int off, enum operand_type ot) {
        switch (ot) {
        case OT_INT:
            g->vmovdqa(r, g->ptr [g->rsp + off]);
            break;
        case OT_FP32:
            g->vmovaps(r, g->ptr [g->rsp + off]);
            break;
        case OT_FP64:
            g->vmovapd(r, g->ptr [g->rsp + off]);
            break;
        }
    }

    void killdep(Xbyak::CodeGenerator *g, Xbyak::Ymm r, enum operand_type ot) {
        switch (ot) {
        case OT_INT:
            g->vpxor(r, r, r);
            break;
        case OT_FP32:
            g->vxorps(r, r, r);
            break;
        case OT_FP64:
            g->vxorpd(r, r, r);
            break;
        }
    }
};



template <>
struct RegMap<Xbyak::Reg64>
{
    const char *name;
    Xbyak::Reg64 v8, v9, v10, v11, v12, v13, v14, v15;

    RegMap()
        :name("reg64"),
         v8(Xbyak::Operand::R8),
         v9(Xbyak::Operand::R9),
         v10(Xbyak::Operand::R10),
         v11(Xbyak::Operand::R11),
         v12(Xbyak::Operand::R12),
         v13(Xbyak::Operand::R13),
         v14(Xbyak::Operand::R14),
         v15(Xbyak::Operand::R15)
        {}

    void save(Xbyak::CodeGenerator *g, Xbyak::Reg64 r, int off, enum operand_type ) {
        g->mov(g->ptr[g->rsp + off], r);
    }

    void restore(Xbyak::CodeGenerator *g, Xbyak::Reg64 r, int off, enum operand_type ) {
        g->mov(r, g->ptr[g->rsp + off]);
    }

    void killdep(Xbyak::CodeGenerator *g, Xbyak::Reg64 r, enum operand_type) {
        g->xor_(r, r);
    }

};

    


int
main(int argc, char **argv)
{
    printf("== latency/throughput ==\n");
    GEN(Reg64, "add", (g->add(dst, src)), false, OT_INT);
    GEN(Reg64, "load", (g->mov(dst, g->ptr[src + g->rdx])), false, OT_INT);


    GEN(Xmm, "pxor", (g->pxor(dst, src)), false, OT_INT);
    GEN(Xmm, "padd", (g->paddd(dst, src)), false, OT_INT);
    GEN(Xmm, "pmuldq", (g->pmuldq(dst, src)), false, OT_INT);

    /* 128 */
    GEN_latency(Xmm, "loadps",
                (g->movaps(dst, g->ptr[g->rdx])),
                (g->movaps(dst, g->ptr[g->rdx + g->rdi])); (g->movq(g->rdi, dst)); ,
                false, OT_INT);

    GEN(Xmm, "xorps", (g->xorps(dst, src)), false, OT_FP32);
    GEN(Xmm, "addps", (g->addps(dst, src)), false, OT_FP32);
    GEN(Xmm, "mulps", (g->mulps(dst, src)), true, OT_FP32);
    GEN(Xmm, "blendps", (g->blendps(dst, src, 0)), false, OT_FP32);
    GEN(Xmm, "pshufb", (g->pshufb(dst, src)), false, OT_INT);
    GEN(Xmm, "pmullw", (g->pmullw(dst, src)), false, OT_INT);
    GEN(Xmm, "phaddd", (g->phaddd(dst, src)), false, OT_INT);

    GEN(Xmm, "pinsrd", (g->pinsrb(dst, g->edx, 0)), false, OT_INT);
    GEN(Xmm, "dpps", (g->dpps(dst, src, 0xff)), false, OT_FP32);
    GEN(Xmm, "cvtps2dq", (g->cvtps2dq(dst, src)), false, OT_FP32);

    /* 256 */
    GEN_latency(Ymm, "loadps",
                (g->vmovaps(dst, g->ptr[g->rdx])),
                (g->vmovaps(dst, g->ptr[g->rdx + g->rdi])); (g->movq(g->rdi, dst)); ,
                false, OT_FP32);

    GEN(Ymm, "xorps", (g->vxorps(dst, dst, src)), false, OT_FP32);
    GEN(Ymm, "mulps", (g->vmulps(dst, dst, src)), true, OT_FP32);
    GEN(Ymm, "addps", (g->vaddps(dst, dst, src)), false, OT_FP32);
    GEN(Ymm, "divps", (g->vdivps(dst, dst, src)), false, OT_FP32);
    GEN(Ymm, "divpd", (g->vdivpd(dst, dst, src)), false, OT_FP64);
    GEN(Ymm, "rsqrtps", (g->vrsqrtps(dst, dst)), false, OT_FP32);
    GEN(Ymm, "rcpps", (g->vrcpps(dst, dst)), false, OT_FP32);
    GEN(Ymm, "sqrtps", (g->vsqrtps(dst, dst)), false, OT_FP32);
    GEN(Ymm, "vperm2f128", (g->vperm2f128(dst,dst,src,0)), false, OT_FP32);
    {
        int reg[4];
        bool have_avx2 = false;
        bool have_fma = false;

#ifdef _WIN32
        __cpuidex(reg, 7, 0);
#else
        __cpuid_count(7, 0, reg[0], reg[1], reg[2], reg[3]);
#endif

        if (reg[1] & (1<<5)) {
            have_avx2 = true;
        }

#ifdef _WIN32
        __cpuid(reg, 1);
#else
        __cpuid(1, reg[0], reg[1], reg[2], reg[3]);
#endif
        if (reg[2] & (1<<12)) {
            have_fma = true;
        }


        if (have_avx2) {
            GEN(Ymm, "pxor", (g->vpxor(dst, dst, src)), false, OT_INT);
            GEN(Ymm, "paddd", (g->vpaddd(dst, dst, src)), false, OT_INT);
            GEN(Ymm, "vpermps", (g->vpermps(dst, dst, src)), false, OT_FP32);
            GEN(Ymm, "vpermpd", (g->vpermpd(dst, dst, 0)), false, OT_FP64);
        }

        if (have_fma) {
            GEN(Ymm, "vfmaps", (g->vfmadd132ps(dst, src, src)), true, OT_FP32);
            GEN(Ymm, "vfmapd", (g->vfmadd132pd(dst, src, src)), true, OT_FP64);
        }
    }
}
#endif