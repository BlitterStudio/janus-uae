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
__gxx_personality_v0
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
_Z10cache_freePh
_Z10do_nothingv
_Z11cache_alloci
_Z12exec_nostatsv
_Z13read_table68kv
_Z14execute_normalv
_Z2auPKc
_Z2uaPKc
_Z7op_illgj
_Z9do_mergesv
_Z9jit_abortPKcz

extern code uses the following jit objects/functions:

_Z10build_compv
_Z12flush_icacheji
_Z13compemu_resetv
_Z13compile_blockP11cpu_historyii
_Z15set_cache_statei
_Z20check_for_cache_missv
_Z24check_prefs_changed_compv
