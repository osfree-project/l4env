/* $Id$
 * Descriptions of the events available for different processor types.
 *
 * Copyright (C) 1999-2003  Mikael Pettersson
 */
#include <stddef.h>	/* for NULL */
#include "libperfctr.h"
#include "event_set.h"

/*
 * Generic events.
 */

static const struct perfctr_event_set generic_event_set = {
    .cpu_type = PERFCTR_X86_GENERIC,
    .event_prefix = NULL,
    .include = NULL,
    .nevents = 0,
    .events = NULL,
};

/*
 * Helper function to translate a cpu_type code to an event_set pointer.
 */

static const struct perfctr_event_set * const cpu_event_set[] = {
    [PERFCTR_X86_GENERIC] = &generic_event_set,
    [PERFCTR_X86_INTEL_P5] = &perfctr_p5_event_set,
    [PERFCTR_X86_INTEL_P5MMX] = &perfctr_p5mmx_event_set,
    [PERFCTR_X86_INTEL_P6] = &perfctr_p6_event_set,
    [PERFCTR_X86_INTEL_PII] = &perfctr_p2_event_set,
    [PERFCTR_X86_INTEL_PIII] = &perfctr_p3_event_set,
    [PERFCTR_X86_CYRIX_MII] = &generic_event_set /*perfctr_mii_event_set*/,
    [PERFCTR_X86_WINCHIP_C6] = &generic_event_set /*perfctr_wcc6_event_set*/,
    [PERFCTR_X86_WINCHIP_2] = &generic_event_set /*perfctr_wc2_event_set*/,
    [PERFCTR_X86_AMD_K7] = &perfctr_k7_event_set,
    [PERFCTR_X86_VIA_C3] = &generic_event_set /*perfctr_vc3_event_set*/,
    [PERFCTR_X86_INTEL_P4] = &generic_event_set /*perfctr_p4_event_set*/,
    [PERFCTR_X86_INTEL_P4M2] = &generic_event_set /*perfctr_p4_event_set*/,
    [PERFCTR_X86_AMD_K8] = &perfctr_k8_event_set,
};

const struct perfctr_event_set *perfctr_cpu_event_set(unsigned cpu_type)
{
    if( cpu_type >= ARRAY_SIZE(cpu_event_set) )
	return 0;
    return cpu_event_set[cpu_type];
}
