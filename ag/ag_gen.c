#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "ag/ag_gen.h"
#include "npr/varray.h"

#define INST_SIZE 4

struct CodeBufferBlock {
#define CODEBUFFER_SIZE 512

    struct CodeBufferBlock *chain;
    size_t cur;
    uint32_t buffer[CODEBUFFER_SIZE];
};

typedef ag_label_id_t label_id_t;

enum label_state {
    LABEL_STATE_EMITTED,
    LABEL_STATE_NOT_EMITTED
};

struct Label {
    enum label_state state;
    uint32_t offset;           /* from top of code */
    char *label_str;           /* strduped */
};

enum labelref_type {
    LABELREF_TYPE_BRANCH, /* low 23bit, offset -8, shift 2 */
};

struct LabelRef {
    enum labelref_type type;
    unsigned int inst_offset;
    label_id_t label_id;
};


static void
alloc_1block(struct CodeBufferBlock **ret, struct CodeBufferBlock *prev)
{
    struct CodeBufferBlock *b = malloc(sizeof(struct CodeBufferBlock));

    b->cur = 0;
    b->chain = prev;

    *ret = b;
}


static void
ag_emit4(struct ag_Emitter *e, uint32_t val)
{
    struct CodeBufferBlock *b = e->last;

    if (b->cur == CODEBUFFER_SIZE) {
        alloc_1block(&e->last, e->last);
    }

    b = e->last;

    b->buffer[b->cur] = val;
    b->cur++;
    e->cur++;
}

#define emit4 ag_emit4

void
ag_emit_bx(struct ag_Emitter *e, enum ag_cond cc, int reg)
{
    ag_emit4(e, (cc << 28) | 0x012fff10 | reg);
}

void
ag_emit_b0(struct ag_Emitter *e, enum ag_cond cc, int l, label_id_t dst)
{
    struct Label *label = VA_ELEM_PTR(struct Label, &e->labels, dst);
    int32_t off = 0;

    if (label->state == LABEL_STATE_EMITTED) {
        off = label->offset - e->cur - 2;
    } else {
        struct LabelRef ref;
        ref.type = LABELREF_TYPE_BRANCH;
        ref.label_id = dst;
        ref.inst_offset = e->cur;

        VA_PUSH(struct LabelRef, &e->label_refs, ref);
    }

    emit4(e, (cc<<28) | (0x5<<25) | (l<<24) | (off&0x00ffffff));
}

void
ag_emit_b(struct ag_Emitter *e, enum ag_cond cc, label_id_t dst)
{
    ag_emit_b0(e, cc, 0, dst);
}
void
ag_emit_bl(struct ag_Emitter *e, enum ag_cond cc, label_id_t dst)
{
    ag_emit_b0(e, cc, 1, dst);
}

void
ag_emit_data_process_reg(struct ag_Emitter *e, enum ag_cond cc, enum ag_data_process_opcode opc,
                         int s, int rd, int shift, int rn, int rm)
{
    emit4(e, (cc<<28) | (opc<<21) | (s<<20) | (rn<<16) | (rd<<12) | (shift<<4) | rm);
}

int
ag_emit_data_process_imm(struct ag_Emitter *e, enum ag_cond cc, enum ag_data_process_opcode opc,
                         int s, int rd, int rn, int32_t imm)
{
    if (imm & 0xffffff00) {
        /* fixme shift */
        return -1;
    }

    emit4(e, (cc<<28) | (1<<25) | (opc<<21) | (s<<20) | (rn<<16) | (rd<<12) | imm);

    return 0;
}

#define DATA_PROCESS(name, opc)                                         \
    void                                                                \
    ag_emit_##name##_reg(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int rm, int shift) \
    {                                                                   \
        ag_emit_data_process_reg(e, cc, opc, s, rd, rn, rm, shift);     \
    }                                                                   \
    int                                                                 \
    ag_emit_##name##_imm(struct ag_Emitter *e, enum ag_cond cc, int s, int rd, int rn, int imm) \
    {                                                                   \
        return ag_emit_data_process_imm(e, cc, opc, s, rd, rn, imm);     \
    }

DATA_PROCESS(and, AG_AND)
DATA_PROCESS(eor, AG_EOR)
DATA_PROCESS(sub, AG_SUB)
DATA_PROCESS(rsb, AG_RSB)
DATA_PROCESS(add, AG_ADD)
DATA_PROCESS(adc, AG_ADC)
DATA_PROCESS(sbc, AG_SBC)
DATA_PROCESS(rsc, AG_RSC)

DATA_PROCESS(tst, AG_TST)
DATA_PROCESS(teq, AG_TEQ)
DATA_PROCESS(cmp, AG_CMP)
DATA_PROCESS(cmn, AG_CMN)
DATA_PROCESS(orr, AG_ORR)
DATA_PROCESS(mov, AG_MOV)
DATA_PROCESS(bic, AG_BIC)
DATA_PROCESS(mvn, AG_MVN)


void
ag_emit_vr3(struct ag_Emitter *e, int opc, int q, int size, int vd, int vn, int vm)
{
    if (q) {
        vd <<= 1;
        vn <<= 1;
        vm <<= 1;
    }

    int dh = (vd >> 4)&1;
    int dl = vd & 0xf;

    int nh = (vn >> 4)&1;
    int nl = vn & 0xf;

    int mh = (vm >> 4)&1;
    int ml = vm & 0xf;

    emit4(e,
          (0xf<<28) | (dh<<22) | (size<<20) | (nl<<16) |
          (dl<<12)   | (opc) | (nh<<7) | (q<<6) | (mh<<5) | (ml));
}


#define IMPL_NEON_VR3_INT(name, opc, prefix)                            \
void                                                                    \
ag_emit_##name##_##prefix##32(struct ag_Emitter *e, int q, int vd, int vn, int vm)   \
{                                                                       \
    ag_emit_vr3(e, opc, q, 2, vd, vn, vm);                             \
}                                                                       \
void                                                                    \
ag_emit_##name##_##prefix##16(struct ag_Emitter *e, int q, int vd, int vn, int vm)    \
{                                                                       \
    ag_emit_vr3(e, opc, q, 1, vd, vn, vm);                             \
}                                                                       \
void                                                                    \
ag_emit_##name##_##prefix##8(struct ag_Emitter *e, int q, int vd, int vn, int vm) \
{                                                                       \
    ag_emit_vr3(e, opc, q, 0, vd, vn, vm);                             \
}

#define IMPL_NEON_VR3_FLT(name, opc)                                    \
void                                                                    \
ag_emit_##name##_f32(struct ag_Emitter *e, int q, int vd, int vn, int vm)   \
{                                                                       \
    ag_emit_vr3(e, opc, q, 2, vd, vn, vm);                             \
}                                                                       \
void                                                                    \
ag_emit_##name##_f64(struct ag_Emitter *e, int q, int vd, int vn, int vm)    \
{                                                                       \
    ag_emit_vr3(e, opc, q, 1, vd, vn, vm);                             \
}



#define IMPL_NEON_VR3_IF(name, opci, opcf)                              \
    IMPL_NEON_VR3_INT(name, opci, i);                                   \
    IMPL_NEON_VR3_FLT(name, opcf);

AG_FOR_EACH_VR3_IF(IMPL_NEON_VR3_IF);

#define IMPL_NEON_VR3_USF(name, opcu, opcs, opcf)                        \
    IMPL_NEON_VR3_INT(name, opcu, u);                                   \
    IMPL_NEON_VR3_INT(name, opcs, s);                                   \
    IMPL_NEON_VR3_FLT(name, opcf);

AG_FOR_EACH_VR3_USF(IMPL_NEON_VR3_USF);

#define IMPL_NEON_VR3_IPF(name, opci, opcp, opcf)                        \
    IMPL_NEON_VR3_INT(name, opci, i);                                   \
    IMPL_NEON_VR3_INT(name, opcp, p);                                   \
    IMPL_NEON_VR3_FLT(name, opcf);                                      \

AG_FOR_EACH_VR3_IPF(IMPL_NEON_VR3_IPF);

#define IMPL_NEON_VR3_USIP(name, opcu, opcs, opci, opcp)                 \
    IMPL_NEON_VR3_INT(name, opcu, u);                                   \
    IMPL_NEON_VR3_INT(name, opcs, s);                                   \
    IMPL_NEON_VR3_INT(name, opci, i);                                   \
    IMPL_NEON_VR3_INT(name, opcp, p);

AG_FOR_EACH_VR3_USIP(IMPL_NEON_VR3_USIP);


void
ag_emit_label(struct ag_Emitter *e, ag_label_id_t label)
{
    struct Label *l = VA_ELEM_PTR(struct Label , &e->labels, label);
    l->offset = e->cur;
    l->state = LABEL_STATE_EMITTED;
}

ag_label_id_t
ag_alloc_label(struct ag_Emitter *e, const char *name /* optional, maybe NULL */)
{
    struct Label label;
    label.state = LABEL_STATE_NOT_EMITTED;
    label.label_str = NULL;
    if (name) {
        label.label_str = strdup(name);
    }

    int ret = e->labels.nelem;
    VA_PUSH(struct Label, &e->labels, label);

    return ret;
}

ag_label_id_t
ag_emit_new_label(struct ag_Emitter *e, const char *name /* optional, maybe NULL */)
{
    ag_label_id_t ret = ag_alloc_label(e, name);
    ag_emit_label(e, ret);
    return ret;
}

void
ag_emitter_init(struct ag_Emitter *e)
{
    e->cur = 0;
    e->code = NULL;
    e->code_size = 0;

    alloc_1block(&e->last, NULL);
    npr_varray_init(&e->labels, 16, sizeof(struct Label));
    npr_varray_init(&e->label_refs, 16, sizeof(struct LabelRef));
}

void
ag_emitter_fini(struct ag_Emitter *e)
{
    if (e->code) {
        munmap(e->code, e->code_size);
    }

    struct CodeBufferBlock *cb = e->last;
    while (cb) {
        struct CodeBufferBlock *cbn = cb->chain;
        free(cb);
        cb = cbn;
    }

    int n = e->labels.nelem;
    struct Label *labels = (struct Label*)e->labels.elements;
    for (int i=0; i<n; i++) {
        if (labels[i].label_str) {
            free(labels[i].label_str);
        }
    }

    npr_varray_discard(&e->labels);
    npr_varray_discard(&e->label_refs);
}

void
ag_alloc_code(void **ret, size_t *ret_size,
              struct ag_Emitter *e)
{
    unsigned int byte_count = 0;
    struct CodeBufferBlock *cb = e->last;
    unsigned int block_count = 0;
    while (cb) {
        byte_count += cb->cur * INST_SIZE;
        block_count++;
        cb = cb->chain;
    }

    if (byte_count == 0) {
        *ret = NULL;
        *ret_size = 0;
        return;
    }

    unsigned int page_size = sysconf(_SC_PAGE_SIZE);
    unsigned int alloc_size = ((byte_count+(page_size-1))/page_size) * page_size;

    unsigned char *p = (unsigned char*)mmap(0, alloc_size,
                                            PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE,
                                            0, 0);

    cb = e->last;

    for (int i=0; i<block_count; i++) {
        int d = (block_count - i)-1;
        size_t offset = d * CODEBUFFER_SIZE * INST_SIZE;

        memcpy(p + offset, cb->buffer, cb->cur * INST_SIZE);

        struct CodeBufferBlock *cbn = cb->chain;
        free(cb);
        cb = cbn;
    }

    e->last = NULL;
    e->code = p;
    e->code_size = alloc_size;

    *ret = p;
    *ret_size = byte_count;

    uint32_t *inst_list = (uint32_t*)p;
    /* resolve label */
    int nref = e->label_refs.nelem;
    for (int ri=0; ri<nref; ri++) {
        struct LabelRef *lr = VA_ELEM_PTR(struct LabelRef, &e->label_refs, ri);
        struct Label *l = VA_ELEM_PTR(struct Label, &e->labels, lr->label_id);
        uint32_t *inst;
        uint32_t inst_val;

        int d = l->offset - lr->inst_offset;

        switch (lr->type) {
        case LABELREF_TYPE_BRANCH:
            inst = &inst_list[lr->inst_offset];
            inst_val = (*inst) & 0xff000000;
            d -= 2;
            inst_val |= d&0x00ffffff;
            *inst = inst_val;
            break;

        }
    }



}
