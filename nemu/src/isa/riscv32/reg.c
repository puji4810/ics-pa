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
#include "local-include/reg.h"

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
  printf("PC: %x\n", cpu.pc);
  printf("Reg:\n");
  size_t reg_len = MUXDEF(CONFIG_RVE, 16, 32);
  for(size_t i = 0; i < reg_len; i++){
    // printf("%-15s0x%-13x\n", reg_name(i), gpr(i));
    printf("%-15s" FMT_REG_DISPLAY "\n", reg_name(i), gpr(i));
  }
  printf("CSR:\n");
  printf("%-15s" FMT_REG_DISPLAY "\n", "mcause", csr.mcause);
  printf("%-15s" FMT_REG_DISPLAY "\n", "mstatus", csr.mstatus);
  printf("%-15s" FMT_REG_DISPLAY "\n", "mtvec", csr.mtvec);
  printf("%-15s" FMT_REG_DISPLAY "\n", "mepc", csr.mepc);
  printf("%-15s" FMT_REG_DISPLAY "\n", "satp", csr.satp);
  printf("%-15s" FMT_REG_DISPLAY "\n", "mscratch", csr.mscratch);
}

word_t isa_reg_str2val(const char *s, bool *success) {
  size_t reg_len = MUXDEF(CONFIG_RVE, 16, 32);
  for(size_t i = 0; i < reg_len; i++){
    const char *name = reg_name(i);
    if(!strcmp(s + 1, "pc")){
      return cpu.pc;      
    }
    if(!strcmp(s + 1, "0") || !strcmp(s + 1, name)){
      return gpr(i);      
    }
  }
  *success = false;
  printf("Can't find reg(%s)\n", s);
  return 0;
}

word_t get_csr(word_t key) {
  switch (key) {
    case CSR_SATP:
      return csr.satp;
      break;
    case CSR_MSTATUS:
      return csr.mstatus;
      break;
    case CSR_MTVEC:
      return csr.mtvec;
      break;
    case CSR_MEPC:
      return csr.mepc;
      break;
    case CSR_MCAUSE:
      return csr.mcause;
      break;
    case CSR_MSCRATCH:
      return csr.mscratch;
      break;
    default:
      panic("CSR GET: Unkown  reg(key="FMT_WORD")", key);
      break;
  }
}

void set_csr(word_t key, word_t value) {
  switch (key) {
    case CSR_SATP:
      csr.satp = value;
      break;
    case CSR_MSTATUS:
      csr.mstatus = value;
      break;
    case CSR_MTVEC:
      csr.mtvec = value;
      break;
    case CSR_MEPC:
      csr.mepc = value;
      break;
    case CSR_MCAUSE:
      csr.mcause = value;
      break;
    case CSR_MSCRATCH:
      csr.mscratch = value;
      break;
    default:
      panic("CSR SET: Unkown  reg(key="FMT_WORD")", key);
      break;
  }
}