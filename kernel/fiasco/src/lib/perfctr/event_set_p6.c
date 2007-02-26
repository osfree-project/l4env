/* $Id$
 * Performance counter event descriptions for the Intel P6 family.
 *
 * Copyright (C) 2003  Mikael Pettersson
 *
 * References
 * ----------
 * [IA32, Volume 3] "Intel Architecture Software Developer's Manual,
 * Volume 3: System Programming Guide". Intel document number 245472-009.
 * (at http://developer.intel.com/)
 */
#include <stddef.h>	/* for NULL */
#include "libperfctr.h"
#include "event_set.h"

/*
 * Intel Pentium Pro (P6) events.
 */

static const struct perfctr_unit_mask_4 p6_um_mesi = {
    { .type = perfctr_um_type_bitmask,
      .default_value = 0x0F,
      .nvalues = 4 },
    { { 0x08, "M (modified cache state)" },
      { 0x04, "E (exclusive cache state)" },
      { 0x02, "S (shared cache state)" },
      { 0x01, "I (invalid cache state)" } }
};

static const struct perfctr_unit_mask_2 p6_um_ebl = {
    { .type = perfctr_um_type_exclusive,
      .default_value = 0x20,
      .nvalues = 2 },
    { { 0x20, "transactions from any processor" },
      { 0x00, "self-generated transactions" } }
};

static const struct perfctr_event p6_events[] = {
    /* Data Cache Unit (DCU) */
    { 0x43, 0x3, NULL, "DATA_MEM_REFS",
      "All loads/stores from/to any memory type" },
    { 0x45, 0x3, NULL, "DCU_LINES_IN", 
      "Total lines allocated in the Data Cache Unit" },
    { 0x46, 0x3, NULL, "DCU_M_LINES_IN",
      "Number of M state lines allocated in the Data Cache Unit" },
    { 0x47, 0x3, NULL, "DCU_M_LINES_OUT",
      "Number of M state lines evicted from the Data Cache Unit" },
    { 0x48, 0x3, NULL, "DCU_MISS_OUTSTANDING", 
      "Weighted cycles while a DCU miss is outstanding"},
    /* Instruction Fetch Unit (IFU) */
    { 0x80, 0x3, NULL, "IFU_IFETCH",
      "All instruction fetches (cacheable and non-cacheable)" },
    { 0x81, 0x3, NULL, "IFU_IFETCH_MISS",
      "Instruction cache fetch misses" },
    { 0x85, 0x3, NULL, "ITLB_MISS",
      "Instruction cache TLB misses" },
    { 0x86, 0x3, NULL, "IFU_MEM_STALL", 
      "Cycles the instruction fetch pipe stage is stalled" },
    { 0x87, 0x3, NULL, "ILD_STALL", 
      "Cycles the instruction length decoder is stalled" },
    /* L2 Cache */
    { 0x28, 0x3, UM(p6_um_mesi), "L2_IFETCH", 
      "L2 instruction fetches" },
    { 0x29, 0x3, UM(p6_um_mesi), "L2_LD", 
      "L2 data loads" },
    { 0x2A, 0x3, UM(p6_um_mesi), "L2_ST",
      "L2 data stores" },
    { 0x24, 0x3, NULL, "L2_LINES_IN", 
      "Lines allocated in L2" },
    { 0x26, 0x3, NULL, "L2_LINES_OUT", 
      "Lines removed from L2" },
    { 0x25, 0x3, NULL, "L2_M_LINES_INM", 
      "Modified lines allocated in L2" },
    { 0x27, 0x3, NULL, "L2_M_LINES_OUTM", 
      "Modified lines removed from L2 for any reason" },
    { 0x2E, 0x3, UM(p6_um_mesi), "L2_RQSTS", 
      "L2 requests" },
    { 0x21, 0x3, NULL, "L2_ADS",
      "L2 address strobes" },
    { 0x22, 0x3, NULL, "L2_DBUS_BUSY", 
      "Cycles L2 cache data bus was busy" },
    { 0x23, 0x3, NULL, "L2_DBUS_BUSY_RD",
      "Cycles data bus was busy in transfer from L2 to CPU" },
    /* External Bus Logic (EBL) */
    { 0x62, 0x3, UM(p6_um_ebl), "BUS_DRDY_CLOCKS",
      "External bus, clocks DRDY (data ready) signal is asserted" },
    { 0x63, 0x3, UM(p6_um_ebl), "BUS_LOCK_CLOCKS",
      "External bus, processor clock cycles LOCK signal is asserted" },
    { 0x60, 0x3, NULL, "BUS_REQ_OUTSTANDING",
      "External bus, outstanding bus requests" },
    { 0x65, 0x3, UM(p6_um_ebl), "BUS_TRAN_BRD",
      "External bus, burst read transactions" },
    { 0x66, 0x3, UM(p6_um_ebl), "BUS_TRAN_RFO",
      "External bus, read-for-ownership transactions" },
    { 0x67, 0x3, UM(p6_um_ebl), "BUS_TRANS_WB",
      "External bus, write-back transactions" },
    { 0x68, 0x3, UM(p6_um_ebl), "BUS_TRAN_IFETCH",
      "External bus, instruction fetch transactions" },
    { 0x69, 0x3, UM(p6_um_ebl), "BUS_TRAN_INVAL",
      "External bus, invalidate transactions" },
    { 0x6A, 0x3, UM(p6_um_ebl), "BUS_TRAN_PWR",
      "External bus, partial write transactions" },
    { 0x6B, 0x3, UM(p6_um_ebl), "BUS_TRANS_P",
      "External bus, partial transactions" },
    { 0x6C, 0x3, UM(p6_um_ebl), "BUS_TRANS_IO",
      "External bus, I/O transactions" },
    { 0x6D, 0x3, UM(p6_um_ebl), "BUS_TRAN_DEF",
      "External bus, deferred transactions" },
    { 0x6E, 0x3, UM(p6_um_ebl), "BUS_TRAN_BURST",
      "External bus, burst transactions" },
    { 0x70, 0x3, UM(p6_um_ebl), "BUS_TRAN_ANY",
      "External bus, all transactions" },
    { 0x6F, 0x3, UM(p6_um_ebl), "BUS_TRAN_MEM",
      "External bus, memory transactions" },
    { 0x64, 0x3, NULL, "BUS_DATA_RCV",
      "Bus clock cycles this processor is receiving data" },
    { 0x61, 0x3, NULL, "BUS_BNR_DRV",
      "Bus clock cycles this processor is driving BNR pin" },
    { 0x7A, 0x3, NULL, "BUS_HIT_DRV",
      "Bus clock cycles this processor is driving HIT pin" },
    { 0x7B, 0x3, NULL, "BUS_HITM_DRV",
      "Bus clock cycles this processor is driving HITM pin" },
    { 0x7E, 0x3, NULL, "BUS_SNOOP_STALL",
      "Bus clock cycles during snoop stall" },
    /* Floating-Point Unit */
    { 0xC1, 0x1, NULL, "FLOPS",
      "Computational FP operations retired" },
    { 0x10, 0x1, NULL, "FP_COMP_OPS_EXE",
      "Computational FP operations executed" },
    { 0x11, 0x2, NULL, "FP_ASSIST",
      "FP exceptions handled by microcode" },
    { 0x12, 0x2, NULL, "MUL",
      "FP multiplies" },
    { 0x13, 0x2, NULL, "DIV",
      "FP divides" },
    { 0x14, 0x1, NULL, "CYCLES_DIV_BUSY",
      "Cycles FP divider is busy" },
    /* Memory Ordering */
    { 0x03, 0x3, NULL, "LD_BLOCKS",
      "Memory ordering, store buffer blocks" },
    { 0x04, 0x3, NULL, "SB_DRAINS",
      "Memory ordering, store buffer drain cycles" },
    { 0x05, 0x3, NULL, "MISALIGN_MEM_REF",
      "Misaligned data memory references (approximate)" },
    /* Instruction Decoding and Retirement */
    { 0xC0, 0x3, NULL, "INST_RETIRED",
      "Instructions retired" },
    { 0xC2, 0x3, NULL, "UOPS_RETIRED",
      "micro-operations retired" },
    { 0xD0, 0x3, NULL, "INST_DECODED",
      "Instructions decoded" },
    /* Interrupts */
    { 0xC8, 0x3, NULL, "HW_INT_RX",
      "Interrupts, hardware interrupts received" },
    { 0xC6, 0x3, NULL, "CYCLES_INT_MASKED",
      "Interrupts, cycles interrupts are disabled" },
    { 0xC7, 0x3, NULL, "CYCLES_INT_PENDING_AND_MASKED",
      "Cycles interrupts are disabled with pending interrupts" },
    /* Branches */
    { 0xC4, 0x3, NULL, "BR_INST_RETIRED",
      "Branches retired" },
    { 0xC5, 0x3, NULL, "BR_MISS_PRED_RETIRED",
      "Mispredicted branches retired" },
    { 0xC9, 0x3, NULL, "BR_TAKEN_RETIRED",
      "Taken branches retired" },
    { 0xCA, 0x3, NULL, "BR_MISS_PRED_TAKEN_RET",
      "Taken mispredicted branches retired" },
    { 0xE0, 0x3, NULL, "BR_INST_DECODED",
      "Branch instructions decoded" },
    { 0xE2, 0x3, NULL, "BTB_MISSES",
      "Branch target buffer misses" },
    { 0xE4, 0x3, NULL, "BR_BOGUS",
      "Bogus branches" },
    { 0xE6, 0x3, NULL, "BACLEARS",
      "BACLEAR assertions (static branch prediction)" },
    /* Stalls */
    { 0xA2, 0x3, NULL, "RESOURCE_STALLS",
      "Cycles during resource-related stalls" },
    { 0xD2, 0x3, NULL, "PARTIAL_RAT_STALLS",
      "Cycles or events for partial stalls" },
    /* Segment Register Loads */
    { 0x06, 0x3, NULL, "SEGMENT_REG_LOADS",
      "Segment register loads" },
    /* Clocks */
    { 0x79, 0x3, NULL, "CPU_CLK_UNHALTED",
      "Cycles processor is not halted" },
};

const struct perfctr_event_set perfctr_p6_event_set = {
    .cpu_type = PERFCTR_X86_INTEL_P6,
    .event_prefix = "P6_",
    .include = NULL,
    .nevents = ARRAY_SIZE(p6_events),
    .events = p6_events,
};

/*
 * Intel Pentium II events.
 * Note that two PII events (0xB0 and 0xCE) are unavailable in the PIII.
 */

static const struct perfctr_unit_mask_0 p2_um_mmx_uops_exec = {
    { .type = perfctr_um_type_fixed,
      .default_value = 0x0F,
      .nvalues = 0 }
};

static const struct perfctr_unit_mask_6 p2_um_mmx_instr_type_exec = {
    { .type = perfctr_um_type_bitmask,
      .default_value = 0x3F,
      .nvalues = 6 },
    { { 0x01, "MMX packed multiplies" },
      { 0x02, "MMX packed shifts" },
      { 0x04, "MMX pack operations" },
      { 0x08, "MMX unpack operations" },
      { 0x10, "MMX packed logical instructions" },
      { 0x20, "MMX packed arithmetic instructions" } }
};

static const struct perfctr_unit_mask_2 p2_um_fp_mmx_trans = {
    { .type = perfctr_um_type_exclusive,
      .default_value = 0x00,
      .nvalues = 2 },
    { { 0x00, "MMX to FP transitions" },
      { 0x01, "FP to MMX transitions" } }
};

static const struct perfctr_unit_mask_4 p2_um_seg_reg_rename = {
    { .type = perfctr_um_type_bitmask,
      .default_value = 0x0F,
      .nvalues = 4 },
    { { 0x01, "segment register ES" },
      { 0x02, "segment register DS" },
      { 0x04, "segment register FS" },
      { 0x08, "segment register GS" } }
};

static const struct perfctr_event p2andp3_events[] = {
    /* MMX Unit */
    { 0xB1, 0x3, NULL, "MMX_SAT_INSTR_EXEC",
      "MMX saturating arithmetic instructions executed" },
    { 0xB2, 0x3, UM(p2_um_mmx_uops_exec), "MMX_UOPS_EXEC",
      "MMX micro-operations executed, port 0..3" },
    { 0xB3, 0x3, UM(p2_um_mmx_instr_type_exec), "MMX_INSTR_TYPE_EXEC",
      "MMX instructions executed" },
    { 0xCC, 0x3, UM(p2_um_fp_mmx_trans), "FP_MMX_TRANS",
      "MMX transitions between FP and MMX states" },
    { 0xCD, 0x3, NULL, "MMX_ASSIST",
      "EMMS instructions executed, SIMD assists" },
    /* Segment Register Renaming */
    { 0xD4, 0x3, UM(p2_um_seg_reg_rename), "SEG_RENAME_STALLS",
      "MMX segment register renaming stalls" },
    { 0xD5, 0x3, UM(p2_um_seg_reg_rename), "SEG_REG_RENAMES",
      "MMX segment register renames" },
    { 0xD6, 0x3, NULL, "RET_SEG_RENAMES",
      "MMX segment register renames retired" },
};

static const struct perfctr_event_set p2andp3_event_set = {
    .cpu_type = PERFCTR_X86_INTEL_PII,
    .event_prefix = "PII_",
    .include = &perfctr_p6_event_set,
    .nevents = ARRAY_SIZE(p2andp3_events),
    .events = p2andp3_events,
};

static const struct perfctr_event p2_events[] = {	/* not in PIII :-( */
    /* MMX Unit */
    { 0xB0, 0x3, NULL, "MMX_INSTR_EXEC",
      "MMX, instructions executed" },
    { 0xCE, 0x3, NULL, "MMX_INSTR_RET",
      "MMX, instructions retired" },
};

const struct perfctr_event_set perfctr_p2_event_set = {
    .cpu_type = PERFCTR_X86_INTEL_PII,
    .event_prefix = "PII_",
    .include = &p2andp3_event_set,
    .nevents = ARRAY_SIZE(p2_events),
    .events = p2_events,
};

/*
 * Intel Pentium III events.
 */

static const struct perfctr_unit_mask_4 p3_um_kni_prefetch = {
    { .type = perfctr_um_type_exclusive,
      .default_value = 0x00,
      .nvalues = 4 },
    { { 0x00, "prefetch NTA" },
      { 0x01, "prefetch T1" },
      { 0x02, "prefetch T2" },
      { 0x03, "weakly ordered stores" } }
};

static const struct perfctr_unit_mask_2 p3_um_kni_inst_retired = {
    { .type = perfctr_um_type_exclusive,
      .default_value = 0x00,
      .nvalues = 2 },
    { { 0x00, "packed and scalar" },
      { 0x01, "scalar" } }
};

static const struct perfctr_event p3_events[] = {
    /* Memory Ordering */
    { 0x07, 0x3, UM(p3_um_kni_prefetch), "EMON_KNI_PREF_DISPATCHED",
      "SSE prefetch instructions dispatched" },
    { 0x4B, 0x3, UM(p3_um_kni_prefetch), "EMON_KNI_PREF_MISS",
      "SSE prefetch instructions dispatched, miss" },
    /* Instruction Decoding and Retirement */
    { 0xD8, 0x3, UM(p3_um_kni_inst_retired), "EMON_KNI_INST_RETIRED",
      "SSE instructions retired" },
    { 0xD9, 0x3, UM(p3_um_kni_inst_retired), "EMON_KNI_COMP_INST_RET",
      "SSE computational instructions retired" },
};

const struct perfctr_event_set perfctr_p3_event_set = {
    .cpu_type = PERFCTR_X86_INTEL_PIII,
    .event_prefix = "PIII_",
    .include = &p2andp3_event_set,
    .nevents = ARRAY_SIZE(p3_events),
    .events = p3_events,
};
