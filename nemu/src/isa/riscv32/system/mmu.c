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
#include <memory/vaddr.h>
#include <memory/paddr.h>

int isa_mmu_check(vaddr_t vaddr, int len, int type) {
  word_t satp = csr.satp;
  bool isSv32 = satp >> 31;
  if (isSv32) {
    return MMU_TRANSLATE;
  } else {
    return MMU_DIRECT;
  }
}

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  word_t satp = csr.satp;
  paddr_t dir_root_guest = (paddr_t)(satp << PAGE_SHIFT);

  VA virtual_guest = (VA)vaddr;
  uint32_t VPN1 = virtual_guest.bitfield.VPN1;
  uint32_t VPN0 = virtual_guest.bitfield.VPN0;
  uint32_t page_offset = virtual_guest.bitfield.page_offset;

  Assert(page_offset + len - 1 <= PAGE_MASK, "MEM_RET_CROSS_PAGE");
  
  paddr_t pte_result_guest = 0;
  int i = 1; // let i = LEVELS − 1
  uint32_t pte_size = sizeof(PTE);
  paddr_t pte_root_guest = dir_root_guest + VPN1 * pte_size;
  PTE *pte_root = (PTE *)guest_to_host(pte_root_guest);
  uint32_t pte_root_v = pte_root->bitfield_detail.V;
  
  Assert(pte_root_v, "the PTE is unvalid");
  uint32_t pte_root_r = pte_root->bitfield_detail.R;
  uint32_t pte_root_x = pte_root->bitfield_detail.X;
  // uint32_t pte_root_w = pte_root->bitfield_detail.W;
  
  if ( (pte_root_r == 1) || (pte_root_x == 1) ) {
    pte_result_guest = pte_root_guest;
    Assert(pte_root->bitfield_detail.PPN0 == 0, "this is a misaligned superpage");
  } else {
    i--;
    paddr_t dir_next_guest = pte_root->bitfield.PPN << PAGE_SHIFT;
    paddr_t pte_next_guest = dir_next_guest + VPN0 * pte_size;
    PTE *pte_next = (PTE *)guest_to_host(pte_next_guest);
    uint32_t pte_next_v = pte_next->bitfield_detail.V;
    Assert(pte_next_v, "the leaf PTE is unvalid");
    uint32_t pte_next_r = pte_next->bitfield_detail.R;
    // uint32_t pte_next_x = pte_next->bitfield_detail.X;
    uint32_t pte_next_w = pte_next->bitfield_detail.W;
    assert(pte_next_r && pte_next_w);
    pte_result_guest = pte_next_guest;
  }

  PTE *pte_result = (PTE *)guest_to_host(pte_result_guest);
  paddr_t pg_paddr_guest = pte_result->bitfield.PPN << PAGE_SHIFT;
  if ( i ) {
    // If i > 0, then this is a superpage translation and pa.ppn[i − 1 : 0] = va.vpn[i − 1 : 0].
    pg_paddr_guest = pg_paddr_guest | (VPN0 << 12);
  }

  return pg_paddr_guest | MEM_RET_OK;
}
