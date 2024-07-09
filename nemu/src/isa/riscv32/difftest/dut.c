/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/difftest.h>
#include "../local-include/reg.h"
#include <memory/paddr.h>
#include <common.h>

static uint32_t csrrw_inst(size_t reg, word_t val, uint32_t rs1, CPU_state *cpu_ref);
static void disassemble_log(uint8_t *ptr);

static void isa_difftest_checkregs_log(const char *reg_name, word_t dut_value, word_t ref_value, bool *is_diff) {
  if(!*is_diff) {
    printf("Difftest:\n");
  }
  printf("%-15s" ANSI_FMT(FMT_REG_DISPLAY, ANSI_FG_RED) ANSI_FMT(FMT_REG_DISPLAY, ANSI_FG_GREEN) "\n", reg_name, dut_value, ref_value);
  *is_diff = true;
}


bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  size_t reg_len = MUXDEF(CONFIG_RVE, 16, 32);
  bool is_diff = false;
  for(size_t i = 0; i < reg_len; i++){
    word_t dut_reg_value = cpu.gpr[i];
    word_t ref_reg_value = ref_r->gpr[i];
    if(dut_reg_value != ref_reg_value){
      isa_difftest_checkregs_log(reg_name(i), dut_reg_value, ref_reg_value, &is_diff);
    }
  }
  vaddr_t dut_pc = cpu.pc;
  vaddr_t ref_pc = ref_r->pc;
  if(dut_pc != ref_pc){
    isa_difftest_checkregs_log("$pc", dut_pc, ref_pc, &is_diff);
  }

  return !is_diff;
}

void isa_difftest_attach() {
  // 这里需要在ref里执行csrrw指令，从而能同步dut的 mstatus，mtvec，mepc，mcause, satp, mscratch的值到 ref
  CPU_state cpu_ref;
  cpu_ref.pc = RESET_VECTOR;

  uint32_t insts[] = {
    csrrw_inst(CSR_MTVEC, csr.mtvec, 1, &cpu_ref),
    csrrw_inst(CSR_MEPC, csr.mepc, 2, &cpu_ref),
    csrrw_inst(CSR_MSTATUS, csr.mstatus, 3, &cpu_ref),
    csrrw_inst(CSR_MCAUSE, csr.mcause, 4, &cpu_ref),
    csrrw_inst(CSR_SATP, csr.satp, 5, &cpu_ref),
    csrrw_inst(CSR_MSCRATCH, csr.mscratch, 6, &cpu_ref),
  };

  ref_difftest_memcpy(RESET_VECTOR, insts, sizeof(insts), DIFFTEST_TO_REF);
  uint32_t steps = sizeof(insts) / sizeof(uint32_t);
  ref_difftest_regcpy(&cpu_ref, DIFFTEST_TO_REF);
  ref_difftest_exec(steps);
}

static uint32_t csrrw_inst(size_t reg, word_t val, uint32_t rs1, CPU_state *cpu_ref) {
  uint32_t inst = 0b001000001110011; // 对应 csrrw指令
  uint32_t rd = 0;
  uint32_t imm = reg;
  inst = inst | (rd << 7) | (imm << 20) | (rs1 << 15);

  disassemble_log((uint8_t *)&inst);
  cpu_ref->gpr[rs1] = val;
  return inst;
}

static void disassemble_log(uint8_t *ptr) {
#ifdef CONFIG_ITRACE
  printf("%x: ", *(uint32_t *)ptr);
  char disassemble_str[1000] = "";
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(disassemble_str, 1000, RESET_VECTOR, ptr, 4);
  printf("%s\n", disassemble_str);
#endif
}