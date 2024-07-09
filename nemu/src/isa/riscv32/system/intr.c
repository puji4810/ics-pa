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
#include "../local-include/reg.h"

#define IRQ_TIMER MCAUSE_MACHINETIMERINTR

static inline void etrace(word_t NO, vaddr_t epc) {
#ifdef CONFIG_ETRACE
    Log(" mcause = "FMT_WORD"(Interrupt = %d, Exception Code = %02d), mepc = "FMT_PADDR, NO, inter_from_mcause(NO), exccode_from_mcause(NO), epc);
#endif
}

word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */
  etrace(NO, epc);

  set_csr(CSR_MEPC, epc);
  set_csr(CSR_MCAUSE, NO);

  // 进入关中断状态
  MSTATUS mstatus = {};
  mstatus.value = csr.mstatus;
  mstatus.bitfield_detail.MPIE = mstatus.bitfield_detail.MIE;
  mstatus.bitfield_detail.MIE = 0;
  set_csr(CSR_MSTATUS, (word_t)mstatus.value);
  return get_csr(CSR_MTVEC);
}

word_t isa_query_intr() {
  MSTATUS mstatus = {0};
  mstatus.value = csr.mstatus;
  if (mstatus.bitfield_detail.MIE && csr.mtvec && cpu.INTR) {
    cpu.INTR = false;
    return IRQ_TIMER;
  }
  return INTR_EMPTY;
}
