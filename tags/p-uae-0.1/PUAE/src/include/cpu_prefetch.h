
STATIC_INLINE uae_u32 get_word_prefetch (int o)
{
	uae_u32 v = regs.irc;
	regs.irc = get_wordi (m68k_getpc () + o);
	return v;
}
STATIC_INLINE uae_u32 get_long_prefetch (int o)
{
	uae_u32 v = get_word_prefetch (o) << 16;
	v |= get_word_prefetch (o + 2);
	return v;
}

#ifdef CPUEMU_20

extern uae_u32 get_word_020_prefetch (int);

STATIC_INLINE uae_u32 next_iword_020 (void)
{
	uae_u32 r = get_word_020_prefetch (0);
	m68k_incpc (2);
	return r;
}
STATIC_INLINE uae_u32 next_ilong_020 (void)
{
	uae_u32 r = next_iword_020 () << 16;
	r |= next_iword_020 ();
	return r;
}

STATIC_INLINE uae_u32 get_long_020_prefetch (int o)
{
	uae_u32 r = get_word_020_prefetch (o) << 16;
	r |= get_word_020_prefetch (o + 2);
	return r;
}

#endif

#ifdef CPUEMU_21

STATIC_INLINE void do_cycles_ce020 (int clocks)
{
	x_do_cycles (clocks * cpucycleunit);
}
STATIC_INLINE void do_cycles_ce020_mem (int clocks, uae_u32 val)
{
	regs.ce020memcycles -= clocks * cpucycleunit;
	x_do_cycles_post (clocks * cpucycleunit, val);
}

STATIC_INLINE void checkcycles_ce020 (void)
{
	if (regs.ce020memcycles > 0)
		x_do_cycles_pre (regs.ce020memcycles);
	regs.ce020memcycles = 0;
}

STATIC_INLINE uae_u32 mem_access_delay_long_read_ce020 (uaecptr addr)
{
	uae_u32 v;
	checkcycles_ce020 ();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		if ((addr & 3) != 0) {
			v  = wait_cpu_cycle_read_ce020 (addr + 0, 1) << 16;
			v |= wait_cpu_cycle_read_ce020 (addr + 2, 1) <<  0;
			return v;
		} else {
			return wait_cpu_cycle_read_ce020 (addr, -1);
		}
	case CE_MEMBANK_FAST:
		v = get_long (addr);
		if ((addr & 3) != 0)
			do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		else
			do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE, v);
		return v;
	case CE_MEMBANK_FAST16BIT:
		v = get_long (addr);
		do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		return v;
	}
	return get_long (addr);
}

STATIC_INLINE uae_u32 mem_access_delay_longi_read_ce020 (uaecptr addr)
{
	uae_u32 v;
	checkcycles_ce020 ();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		if ((addr & 3) != 0) {
			v  = wait_cpu_cycle_read_ce020 (addr + 0, 1) << 16;
			v |= wait_cpu_cycle_read_ce020 (addr + 2, 1) <<  0;
			return v;
		} else {
			return wait_cpu_cycle_read_ce020 (addr, -1);
		}
	case CE_MEMBANK_FAST:
		v = get_longi (addr);
		if ((addr & 3) != 0)
			do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		else
			do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE, v);
		return v;
	case CE_MEMBANK_FAST16BIT:
		v = get_longi (addr);
		do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		return v;
	}
	return get_longi (addr);
}

STATIC_INLINE uae_u32 mem_access_delay_word_read_ce020 (uaecptr addr)
{
	uae_u32 v;
	checkcycles_ce020 ();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		if ((addr & 3) == 3) {
			v  = wait_cpu_cycle_read_ce020 (addr + 0, 0) << 8;
			v |= wait_cpu_cycle_read_ce020 (addr + 1, 0) << 0;
			return v;
		} else {
			return wait_cpu_cycle_read_ce020 (addr, 1);
		}
	case CE_MEMBANK_FAST:
	case CE_MEMBANK_FAST16BIT:
		v = get_word (addr);
		if ((addr & 3) == 3)
			do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		else
			do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE, v);
		return v;
	}
	return get_word (addr);
}

STATIC_INLINE uae_u32 mem_access_delay_wordi_read_ce020 (uaecptr addr)
{
	uae_u32 v;
	checkcycles_ce020 ();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		return wait_cpu_cycle_read_ce020 (addr, 1);
	case CE_MEMBANK_FAST:
	case CE_MEMBANK_FAST16BIT:
		v = get_wordi (addr);
		do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE, v);
		return v;
	}
	return get_wordi (addr);
}

STATIC_INLINE uae_u32 mem_access_delay_byte_read_ce020 (uaecptr addr)
{
	uae_u32 v;
	checkcycles_ce020 ();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		return wait_cpu_cycle_read_ce020 (addr, 0);
	case CE_MEMBANK_FAST:
	case CE_MEMBANK_FAST16BIT:
		v = get_byte (addr);
		do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE, v);
		return v;

	}
	return get_byte (addr);
}

STATIC_INLINE void mem_access_delay_byte_write_ce020 (uaecptr addr, uae_u32 v)
{
	checkcycles_ce020 ();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		wait_cpu_cycle_write_ce020 (addr, 0, v);
		return;
	case CE_MEMBANK_FAST:
	case CE_MEMBANK_FAST16BIT:
		put_byte (addr, v);
		do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE, v);
		return;
	}
	put_byte (addr, v);
}

STATIC_INLINE void mem_access_delay_word_write_ce020 (uaecptr addr, uae_u32 v)
{
	checkcycles_ce020 ();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		if ((addr & 3) == 3) {
			wait_cpu_cycle_write_ce020 (addr + 0, 0, (v >> 8) & 0xff);
			wait_cpu_cycle_write_ce020 (addr + 1, 0, (v >> 0) & 0xff);
		} else {
			wait_cpu_cycle_write_ce020 (addr + 0, 1, v);
		}
		return;
		break;
	case CE_MEMBANK_FAST:
	case CE_MEMBANK_FAST16BIT:
		put_word (addr, v);
		if ((addr & 3) == 3)
			do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		else
			do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE, v);
		return;
	}
	put_word (addr, v);
}

STATIC_INLINE void mem_access_delay_long_write_ce020 (uaecptr addr, uae_u32 v)
{
	checkcycles_ce020 ();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		if ((addr & 3) == 3) {
			wait_cpu_cycle_write_ce020 (addr + 0, 1, (v >> 16) & 0xffff);
			wait_cpu_cycle_write_ce020 (addr + 2, 1, (v >>  0) & 0xffff);
		} else {
			wait_cpu_cycle_write_ce020 (addr + 0, -1, v);
		}
		return;
		break;
	case CE_MEMBANK_FAST:
		put_long (addr, v);
		if ((addr & 3) != 0)
			do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		else
			do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE, v);
		return;
	case CE_MEMBANK_FAST16BIT:
		put_long (addr, v);
		do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		return;
	}
	put_long (addr, v);
}

STATIC_INLINE uae_u32 get_long_ce020 (uaecptr addr)
{
	return mem_access_delay_long_read_ce020 (addr);
}
STATIC_INLINE uae_u32 get_word_ce020 (uaecptr addr)
{
	return mem_access_delay_word_read_ce020 (addr);
}
STATIC_INLINE uae_u32 get_byte_ce020 (uaecptr addr)
{
	return mem_access_delay_byte_read_ce020 (addr);
}

STATIC_INLINE void put_long_ce020 (uaecptr addr, uae_u32 v)
{
	mem_access_delay_long_write_ce020 (addr, v);
}
STATIC_INLINE void put_word_ce020 (uaecptr addr, uae_u32 v)
{
	mem_access_delay_word_write_ce020 (addr, v);
}
STATIC_INLINE void put_byte_ce020 (uaecptr addr, uae_u32 v)
{
	mem_access_delay_byte_write_ce020 (addr, v);
}

extern uae_u32 get_word_ce020_prefetch (int);

STATIC_INLINE uae_u32 get_long_ce020_prefetch (int o)
{
	uae_u32 v;
	v = get_word_ce020_prefetch (o) << 16;
	v |= get_word_ce020_prefetch (o + 2);
	return v;
}

STATIC_INLINE uae_u32 next_iword_020ce (void)
{
	uae_u32 r = get_word_ce020_prefetch (0);
	m68k_incpc (2);
	return r;
}
STATIC_INLINE uae_u32 next_ilong_020ce (void)
{
	uae_u32 r = get_long_ce020_prefetch (0);
	m68k_incpc (4);
	return r;
}



STATIC_INLINE void m68k_do_bsr_ce020 (uaecptr oldpc, uae_s32 offset)
{
	m68k_areg (regs, 7) -= 4;
	x_put_long (m68k_areg (regs, 7), oldpc);
	m68k_incpc (offset);
}
STATIC_INLINE void m68k_do_rts_ce020 (void)
{
	m68k_setpc (x_get_long (m68k_areg (regs, 7)));
	m68k_areg (regs, 7) += 4;
}


#endif

#ifdef CPUEMU_22

extern uae_u32 get_word_ce030_prefetch (int);
extern void write_dcache030 (uaecptr, uae_u32, int);
extern uae_u32 read_dcache030 (uaecptr, int);

STATIC_INLINE void put_long_ce030 (uaecptr addr, uae_u32 v)
{
	write_dcache030 (addr, v, 2);
	mem_access_delay_long_write_ce020 (addr, v);
}
STATIC_INLINE void put_word_ce030 (uaecptr addr, uae_u32 v)
{
	write_dcache030 (addr, v, 1);
	mem_access_delay_word_write_ce020 (addr, v);
}
STATIC_INLINE void put_byte_ce030 (uaecptr addr, uae_u32 v)
{
	write_dcache030 (addr, v, 0);
	mem_access_delay_byte_write_ce020 (addr, v);
}
STATIC_INLINE uae_u32 get_long_ce030 (uaecptr addr)
{
	return read_dcache030 (addr, 2);
}
STATIC_INLINE uae_u32 get_word_ce030 (uaecptr addr)
{
	return read_dcache030 (addr, 1);
}
STATIC_INLINE uae_u32 get_byte_ce030 (uaecptr addr)
{
	return read_dcache030 (addr, 0);
}

STATIC_INLINE uae_u32 get_long_ce030_prefetch (int o)
{
	uae_u32 v;
	v = get_word_ce030_prefetch (o) << 16;
	v |= get_word_ce030_prefetch (o + 2);
	return v;
}

STATIC_INLINE uae_u32 next_iword_030ce (void)
{
	uae_u32 r = get_word_ce030_prefetch (0);
	m68k_incpc (2);
	return r;
}
STATIC_INLINE uae_u32 next_ilong_030ce (void)
{
	uae_u32 r = get_long_ce030_prefetch (0);
	m68k_incpc (4);
	return r;
}

STATIC_INLINE void m68k_do_bsr_ce030 (uaecptr oldpc, uae_s32 offset)
{
	m68k_areg (regs, 7) -= 4;
	put_long_ce030 (m68k_areg (regs, 7), oldpc);
	m68k_incpc (offset);
}
STATIC_INLINE void m68k_do_rts_ce030 (void)
{
	m68k_setpc (get_long_ce030 (m68k_areg (regs, 7)));
	m68k_areg (regs, 7) += 4;
}

#endif

#ifdef CPUEMU_12

STATIC_INLINE void do_cycles_ce000 (int clocks)
{
	x_do_cycles (clocks * cpucycleunit);
}

STATIC_INLINE void ipl_fetch (void)
{
	regs.ipl = regs.ipl_pin;
}

STATIC_INLINE uae_u32 mem_access_delay_word_read (uaecptr addr)
{
        uae_u32 v;
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		return wait_cpu_cycle_read (addr, 1);
	case CE_MEMBANK_FAST:
	case CE_MEMBANK_FAST16BIT:
		v = get_word (addr);
		x_do_cycles_post (4 * cpucycleunit, v);
		return v;
	}
	return get_word (addr);
}
STATIC_INLINE uae_u32 mem_access_delay_wordi_read (uaecptr addr)
{
        uae_u32 v;
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		return wait_cpu_cycle_read (addr, 1);
	case CE_MEMBANK_FAST:
	case CE_MEMBANK_FAST16BIT:
		v = get_wordi (addr);
		x_do_cycles_post (4 * cpucycleunit, v);
		return v;
	}
	return get_wordi (addr);
}

STATIC_INLINE uae_u32 mem_access_delay_byte_read (uaecptr addr)
{
        uae_u32 v;
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		return wait_cpu_cycle_read (addr, 0);
	case CE_MEMBANK_FAST:
	case CE_MEMBANK_FAST16BIT:
		v = get_byte (addr);
		x_do_cycles_post (4 * cpucycleunit, v);
		return v;
	}
	return get_byte (addr);
}
STATIC_INLINE void mem_access_delay_byte_write (uaecptr addr, uae_u32 v)
{
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		wait_cpu_cycle_write (addr, 0, v);
		return;
	case CE_MEMBANK_FAST:
	case CE_MEMBANK_FAST16BIT:
		put_byte (addr, v);
		x_do_cycles_post (4 * cpucycleunit, v);
		return;
	}
	put_byte (addr, v);
}
STATIC_INLINE void mem_access_delay_word_write (uaecptr addr, uae_u32 v)
{
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		wait_cpu_cycle_write (addr, 1, v);
		return;
	case CE_MEMBANK_FAST:
	case CE_MEMBANK_FAST16BIT:
		put_word (addr, v);
		x_do_cycles_post (4 * cpucycleunit, v);
		return;
	}
	put_word (addr, v);
}

STATIC_INLINE uae_u32 get_long_ce000 (uaecptr addr)
{
	uae_u32 v = mem_access_delay_word_read (addr) << 16;
	v |= mem_access_delay_word_read (addr + 2);
	return v;
}
STATIC_INLINE uae_u32 get_word_ce000 (uaecptr addr)
{
	return mem_access_delay_word_read (addr);
}
STATIC_INLINE uae_u32 get_wordi_ce000 (int offset)
{
	return mem_access_delay_wordi_read (m68k_getpc () + offset);
}
STATIC_INLINE uae_u32 get_byte_ce000 (uaecptr addr)
{
	return mem_access_delay_byte_read (addr);
}
STATIC_INLINE uae_u32 get_word_ce000_prefetch (int o)
{
	uae_u32 v = regs.irc;
	regs.irc = x_get_iword (o);
	return v;
}

STATIC_INLINE void put_long_ce000 (uaecptr addr, uae_u32 v)
{
	mem_access_delay_word_write (addr, v >> 16);
	mem_access_delay_word_write (addr + 2, v);
}
STATIC_INLINE void put_word_ce000 (uaecptr addr, uae_u32 v)
{
	mem_access_delay_word_write (addr, v);
}
STATIC_INLINE void put_byte_ce000 (uaecptr addr, uae_u32 v)
{
	mem_access_delay_byte_write (addr, v);
}

STATIC_INLINE void m68k_do_rts_ce (void)
{
	uaecptr pc;
	pc = x_get_word (m68k_areg (regs, 7)) << 16;
	pc |= x_get_word (m68k_areg (regs, 7) + 2);
	m68k_areg (regs, 7) += 4;
	if (pc & 1)
		exception3 (0x4e75, pc);
	else
		m68k_setpc (pc);
}

STATIC_INLINE void m68k_do_bsr_ce (uaecptr oldpc, uae_s32 offset)
{
	m68k_areg (regs, 7) -= 4;
	x_put_word (m68k_areg (regs, 7), oldpc >> 16);
	x_put_word (m68k_areg (regs, 7) + 2, oldpc);
	m68k_incpc (offset);
}

STATIC_INLINE void m68k_do_jsr_ce (uaecptr oldpc, uaecptr dest)
{
	m68k_areg (regs, 7) -= 4;
	x_put_word (m68k_areg (regs, 7), oldpc >> 16);
	x_put_word (m68k_areg (regs, 7) + 2, oldpc);
	m68k_setpc (dest);
}

#endif

STATIC_INLINE uae_u32 get_disp_ea_000 (uae_u32 base, uae_u32 dp)
{
	int reg = (dp >> 12) & 15;
	uae_s32 regd = regs.regs[reg];
	if ((dp & 0x800) == 0)
		regd = (uae_s32)(uae_s16)regd;
	return base + (uae_s8)dp + regd;
}
