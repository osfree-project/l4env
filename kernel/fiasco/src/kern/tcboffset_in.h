  DUMP_MEMBER1 (THREAD, Context, _state,		STATE)
  DUMP_MEMBER1 (THREAD, Context, _kernel_sp,		KERNEL_SP)
  DUMP_MEMBER1 (THREAD, Context, _donatee,		DONATEE)
  DUMP_MEMBER1 (THREAD, Context, _lock_cnt,		LOCK_CNT)
  DUMP_MEMBER1 (THREAD, Context, _thread_lock,		THREAD_LOCK_PTR)
  DUMP_MEMBER1 (THREAD, Context, _sched_context,	SCHED_CONTEXT)
  DUMP_MEMBER1 (THREAD, Context, _sched,		SCHED)
  DUMP_MEMBER1 (THREAD, Context, _period,		PERIOD)
  DUMP_MEMBER1 (THREAD, Context, _mode,			MODE)
  DUMP_MEMBER1 (THREAD, Context, _mcp,			MCP)
  DUMP_MEMBER1 (THREAD, Context, _fpu_state,		FPU_STATE)
  DUMP_MEMBER1 (THREAD, Context, _consumed_time,	CONSUMED_TIME)
  DUMP_MEMBER1 (THREAD, Context, _ready_next,		READY_NEXT)
  DUMP_MEMBER1 (THREAD, Context, _ready_prev,		READY_PREV)
#if defined(CONFIG_IA32) && !defined(CONFIG_PF_UX) && defined(CONFIG_HANDLE_SEGMENTS)
  DUMP_MEMBER1 (THREAD, Context, _gdt_tls,		GDT_TLS)
#endif
  DUMP_MEMBER1 (THREAD, Receiver, _partner,		PARTNER)
  DUMP_MEMBER1 (THREAD, Receiver, _rcv_regs,		RCV_REGS)
  DUMP_MEMBER1 (THREAD, Receiver, _pagein_addr,		PAGEIN_ADDR)
  DUMP_MEMBER1 (THREAD, Receiver, _pagein_error_code,	PAGEIN_ERROR_CODE)
  DUMP_MEMBER1 (THREAD, Receiver, _pagein_applicant,	PAGEIN_APPLICANT)
  DUMP_MEMBER1 (THREAD, Receiver, _sender_list._head,	SENDER_FIRST)
  DUMP_MEMBER1 (THREAD, Thread, _timeout,		TIMEOUT)
  DUMP_MEMBER1 (THREAD, Thread, _id,			ID)
  DUMP_MEMBER1 (THREAD, Thread, _receiver,		RECEIVER)
  DUMP_MEMBER1 (THREAD, Thread, _preemption,		PREEMPTION)
  DUMP_MEMBER1 (THREAD, Thread, _preemption._id,	PREEMPTION__ID)
  DUMP_MEMBER1 (THREAD, Thread,	_preemption._receiver,
                PREEMPTION__RECEIVER)
  DUMP_MEMBER1 (THREAD, Thread,	_preemption._pending,
                PREEMPTION__PENDING)
  DUMP_MEMBER1 (THREAD, Thread, _task,		SPACE)
  DUMP_MEMBER1 (THREAD, Thread, _thread_lock,	THREAD_LOCK)
  DUMP_MEMBER1 (THREAD, Thread,	_thread_lock._lock_owner,
                THREAD_LOCK__SWITCH_LOCK__LOCK_OWNER)
  DUMP_MEMBER1 (THREAD, Thread,	_thread_lock._switch_hint,
                THREAD_LOCK__SWITCH_HINT)
  DUMP_MEMBER1 (THREAD, Thread, _pager,			PAGER)
  DUMP_MEMBER1 (THREAD, Thread, _ext_preempter,		EXT_PREEMPTER)
  DUMP_MEMBER1 (THREAD, Thread, present_next,		PRESENT_NEXT)
  DUMP_MEMBER1 (THREAD, Thread, present_prev,		PRESENT_PREV)
  DUMP_MEMBER1 (THREAD, Thread, _irq,			IRQ)
  DUMP_MEMBER1 (THREAD, Thread, _target_desc,		TARGET_DESC)
  DUMP_MEMBER1 (THREAD, Thread, _pagein_status_code,	PAGEIN_STATUS_CODE)
  DUMP_MEMBER1 (THREAD, Thread, _vm_window0,		VM_WINDOW0)
  DUMP_MEMBER1 (THREAD, Thread, _vm_window1,		VM_WINDOW1)
  DUMP_MEMBER1 (THREAD, Thread, _recover_jmpbuf,	RECOVER_JMPBUF)
  DUMP_MEMBER1 (THREAD, Thread, _pf_timeout,		PF_TIMEOUT)
  DUMP_MEMBER1 (THREAD, Thread, _last_pf_address,	LAST_PF_ADDRESS)
  DUMP_MEMBER1 (THREAD, Thread, _last_pf_error_code,	LAST_PF_ERROR_CODE)
  DUMP_MEMBER1 (THREAD, Thread, _magic,			MAGIC)
#if defined(CONFIG_IA32)
  DUMP_MEMBER1 (THREAD, Thread, _idt,			IDT)
  DUMP_MEMBER1 (THREAD, Thread, _idt_limit,		IDT_LIMIT)
#endif
  DUMP_OFFSET  (THREAD, MAX, sizeof (Thread))

  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_owner,		OWNER)
  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_id,		ID)
  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_prio,		PRIO)
  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_quantum,		QUANTUM)
  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_left	,	LEFT)
  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_preemption_time,	PREEMPTION_TIME)
  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_prev,		PREV)
  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_next,		NEXT)
  DUMP_OFFSET  (SCHED_CONTEXT, MAX, sizeof (Sched_context))

  DUMP_MEMBER1 (SPACE,         Space, _mem_space,               MEM_SPACE)
  DUMP_MEMBER1 (MEM_SPACE, Mem_space, _dir,                     PGTABLE)

  DUMP_MEMBER1 (IRQ, Irq, _queued,				QUEUED)

  DUMP_MEMBER1 (TBUF_STATUS, Tracebuffer_status, kerncnts,      KERNCNTS)
  
  DUMP_MEMBER1 (THREAD, Thread, _exc_ip,	        EXCEPTION_IP)

  DUMP_CAST_OFFSET (Thread, Receiver)
  DUMP_CAST_OFFSET (Thread, Sender)

  DUMP_CONSTANT (MEM_LAYOUT__TCBS,             Mem_layout::Tcbs)
#ifdef CONFIG_IA32
  DUMP_CONSTANT (MEM_LAYOUT__PHYSMEM,          Mem_layout::Physmem)
  DUMP_CONSTANT (MEM_LAYOUT__TBUF_STATUS_PAGE, Mem_layout::Tbuf_status_page)
#endif
#ifdef CONFIG_PF_PC
  DUMP_CONSTANT (MEM_LAYOUT__SMAS_START,       Mem_layout::Smas_start)
  DUMP_CONSTANT (MEM_LAYOUT__SMAS_END,         Mem_layout::Smas_end)
  DUMP_CONSTANT (MEM_LAYOUT__SMAS_VERSION,     Mem_layout::Smas_version)
  DUMP_CONSTANT (MEM_LAYOUT__SMAS_AREA,        Mem_layout::Smas_area)
  DUMP_CONSTANT (MEM_LAYOUT__SMAS_TRAMPOLINE,  Mem_layout::Smas_trampoline)
  DUMP_CONSTANT (MEM_LAYOUT__IPC_WINDOW0,      Mem_layout::Ipc_window0)
  DUMP_CONSTANT (MEM_LAYOUT__IPC_WINDOW1,      Mem_layout::Ipc_window1)
  DUMP_CONSTANT (MEM_LAYOUT__IO_BITMAP,        Mem_layout::Io_bitmap)
  DUMP_CONSTANT (MEM_LAYOUT__SMAS_IO_BMAP_BAK, Mem_layout::Smas_io_bmap_bak)
  DUMP_CONSTANT (MEM_LAYOUT__SYSCALLS,         Mem_layout::Syscalls)
#endif
#ifdef CONFIG_PF_UX
  DUMP_CONSTANT (MEM_LAYOUT__TRAMPOLINE_PAGE,  Mem_layout::Trampoline_page)
#endif

