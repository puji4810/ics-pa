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

#ifndef __ISA_RISCV_H__
#define __ISA_RISCV_H__

#include <common.h>

typedef struct {
  word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
  vaddr_t pc;
  bool INTR;
} MUXDEF(CONFIG_RV64, riscv64_CPU_state, riscv32_CPU_state);

typedef struct {
  word_t mstatus;
  word_t mtvec;
  word_t mepc;
  word_t mcause;
  word_t satp;
  word_t mscratch;
} MUXDEF(CONFIG_RV64, riscv64_CPU_csr, riscv32_CPU_csr);

// decode
typedef struct {
  union {
    uint32_t val;
  } inst;
} MUXDEF(CONFIG_RV64, riscv64_ISADecodeInfo, riscv32_ISADecodeInfo);

// #define isa_mmu_check(vaddr, len, type) (MMU_DIRECT)

typedef union {
  struct {
    uint32_t page_offset : 12;
    uint32_t VPN0 : 10;
    uint32_t VPN1 : 10;
  } bitfield;
  uint32_t value;
} RISCV32_VA;

typedef union {
  struct {
    uint32_t V : 1;
    uint32_t R : 1;
    uint32_t W : 1;
    uint32_t X : 1;
    uint32_t U : 1;
    uint32_t G : 1;
    uint32_t A : 1;
    uint32_t D : 1;
    uint32_t RSW : 1;
    uint32_t PPN0 : 10;
    uint32_t PPN1 : 12;
  } bitfield_detail;
  struct {
    uint32_t flag : 10;
    uint32_t PPN : 22;
  } bitfield;
  uint32_t value;
} RISCV32_PTE;

typedef RISCV32_VA VA;
typedef RISCV32_PTE PTE;

typedef union {
  struct {
    uint32_t WPRI_part0 : 1;
    uint32_t SIE : 1;
    uint32_t WPRI_part1 : 1;
    uint32_t MIE : 1;
    uint32_t WPRI_part2 : 1;
    uint32_t SPIE : 1;
    uint32_t UBE : 1;
    uint32_t MPIE : 1;
    uint32_t SPP : 1;
    uint32_t VS : 2;
    uint32_t MPP : 2;
    uint32_t FS : 2;
    uint32_t XS : 2;
    uint32_t MPRV : 2;
    uint32_t SUM : 1;
    uint32_t MXR : 1;
    uint32_t TVM : 1;
    uint32_t TW : 1;
    uint32_t TSR : 1;
    uint32_t WPRI_part3 : 8;
    uint32_t SD : 1;
  } bitfield_detail;
  uint32_t value;
} RISCV32_MSTATUS;

typedef RISCV32_MSTATUS MSTATUS;

#endif
