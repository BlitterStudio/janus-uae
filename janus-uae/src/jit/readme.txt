To get a feeling, how the communication between the jit and the main-exe works..

jit objects call the following functions and use the following extern objects:

areg_byteinc
__assert_fail
baseaddr
canbang
changed_prefs
comp_pc_p
cpufunctbl
currprefs
failure
fp_1e1
fp_1e2
fp_1e4
fp_1e8
imm8_table
kickmem_bank
lookuptab
m68k_pc_offset
mem_banks
natmem_offset
needed_flags
nr_cpuop_funcs
op_smalltbl_0_comp_ff
op_smalltbl_0_comp_nf
pissoff
regflags
regs
special_mem
start_pc
start_pc_p
table68k
xhex_1e1024
xhex_1e128
xhex_1e16
xhex_1e2048
xhex_1e256
xhex_1e32
xhex_1e4096
xhex_1e512
xhex_1e64
xhex_exp_1
xhex_l10_2
xhex_l10_e
xhex_ln_10
cache_free(unsigned char*)
do_nothing()
cache_alloc(int)
exec_nostats()
read_table68k()
execute_normal()
au(char const*)
ua(char const*)
op_illg(unsigned int)
do_merges()
jit_abort(char const*, ...)

extern code uses the following jit objects/functions:

build_comp()
flush_icache(unsigned int, int)
compemu_reset()
compile_block(cpu_history*, int, int)
set_cache_state(int)
check_for_cache_miss()
check_prefs_changed_comp()
