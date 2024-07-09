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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  uint32_t pre_val;
  char *expression;
  /* TODO: Add more members if necessary */

} WP;

static WP* insert_list (WP *wp, WP **list_ptr);
static WP* delete_list (int no, WP **list_ptr);
static size_t size_list(WP *list);

static int wp_no = 1;
static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = 0;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    free(wp_pool[i].expression);
    wp_pool[i].expression = NULL;
    wp_pool[i].pre_val = 0;
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

bool new_wp(char *expression) {
  if (expression == NULL) {
    printf("expression is NULL\n");
    return false;
  }  
  if (free_ == NULL) {
    printf("free_ is NULL\n");
    return false;
  }
  bool success = true;
  uint32_t val = expr(expression, &success);
  if(!success){
    return false;
  }
  int first_no = free_->NO;
  WP* wp = delete_list(first_no, &free_);
  if (wp == NULL){
    printf("Cann't get wp from free_\n");
    return false;
  }
  wp->NO = wp_no;
  wp_no++;
  wp->next = NULL;
  wp->pre_val = val;
  size_t expression_str_len = strlen(expression) + 1;
  wp->expression = malloc(expression_str_len);
  assert(wp->expression);
  strncpy(wp->expression, expression, expression_str_len);
  WP* insert_result = insert_list(wp, &head);
  assert(insert_result);
  printf("watchpoint %d: %s\n", wp->NO, wp->expression);
  assert(size_list(head) + size_list(free_) == NR_WP);
  return true;
}

void free_wp(int no) {
  assert(no >= 0);
  if (head == NULL) {
    return;
  }
  WP* wp = delete_list(no, &head);
  if (wp == NULL){
    return;
  }
  wp->NO = 0;
  wp->next = NULL;
  wp->pre_val = 0;
  free(wp->expression);
  wp->expression = NULL;
  WP* insert_result = insert_list(wp, &free_);
  assert(insert_result);
  assert(size_list(head) + size_list(free_) == NR_WP);
  return;
}

bool check_wps(){
  WP *cur = head;
  bool is_changed = false;
  while (cur) {
    char *expression = cur->expression;
    bool success = 1;
    uint32_t val = expr(expression, &success);
    assert(success);
    if(cur->pre_val != val){
      printf("Watchpoint %d: %s\n", cur->NO, cur->expression);
      printf("Old value = %d (0x%x)\n", cur->pre_val, cur->pre_val);
      printf("New value = %d (0x%x)\n", val, val);
      cur->pre_val = val;
      is_changed = true;
    }
    cur = cur->next;
  }
  return is_changed;
}

void printf_wps(){
  if ( !head ) {
    printf("No watchpoints.\n");
    return;
  }
  WP *cur = head;
  printf("Num     What\n");
  while (cur) {
    char *expression = cur->expression;
    printf("%-8d%s\n", cur->NO, expression);
    cur = cur->next;
  }
}

static WP* insert_list (WP *wp, WP **list_ptr) {
  WP *list = *list_ptr;
  if ((list == NULL) || (wp->NO < list->NO)){
    wp->next = list;
    *list_ptr = wp;
    return wp;
  }
  WP *pre = list;
  WP *cur = list->next;
  while (cur) {
    if( wp->NO <= cur->NO ){
      pre->next = wp;
      wp->next = cur;
      return wp;
    }
    pre = cur;
    cur = cur->next;
  }
  pre->next = wp;
  wp->next = NULL;
  return wp;
}

static WP* delete_list (int no, WP **list_ptr) {
  WP *list = *list_ptr;
  if (list == NULL){
    return NULL;
  }
  if (list->NO == no){
    *list_ptr = list->next;
    return list;
  }
  WP *pre = list;
  WP *cur = list->next;
  while (cur) {
    if( cur->NO == no ){
      pre->next = cur->next;
      return cur;
    }
    pre = cur;
    cur = cur->next;
  }
  return NULL;
}

static size_t size_list(WP *list) {
  size_t size = 0;
  WP *cur = list;
  while (cur) {
    size++;
    cur = cur->next;
  }
  return size;
}