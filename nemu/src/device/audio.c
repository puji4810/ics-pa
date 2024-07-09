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

#include <common.h>
#include <device/map.h>
#include <SDL2/SDL.h>

enum {
  reg_freq,
  reg_channels,
  reg_samples,
  reg_sbuf_size,
  reg_init,
  reg_count,      // 流缓冲区有待被SDL库读取的大小，也就是 reg_sbuf_right - reg_sbuf_left
  reg_sbuf_right, // 把流缓冲区看作队列，右进左出，记录右进的所有字节大小
  reg_sbuf_left,  // 同上，记录左出的所有字节大小
  nr_reg
};

static uint8_t *sbuf = NULL;
static uint32_t *audio_base = NULL;
static int freq = 0;
static uint8_t channels = 0;
static uint16_t samples = 0;
static uint32_t sbuf_size = CONFIG_SB_SIZE;
static uint32_t sbuf_right = 0; // 待写入位置
static uint32_t sbuf_left = 0;  // 待读取位置


void audio_callback ( void*  userdata, Uint8* stream, int len ) {
  int nread = len;
  int count = sbuf_right - sbuf_left;
  if (count < len) {
    nread = count;
  }
  int b = 0;
  while( b < nread) {
    uint8_t* address = sbuf + ( sbuf_left + b) % sbuf_size;
    *(stream + b) = *address;
    b++;
  }
  sbuf_left = sbuf_left + nread;
  if (len > nread) {
    memset(stream + nread, 0, len - nread);
  }
}

static void audio_init(int freq, uint8_t channels, uint16_t samples) {
  SDL_AudioSpec s = {};
  SDL_memset(&s, 0, sizeof(s));
  s.freq = freq;
  s.format = AUDIO_S16SYS;  // 假设系统中音频数据的格式总是使用16位有符号数来表示
  s.userdata = NULL;        // 不使用
  s.channels = channels;
  s.samples = samples;
  s.callback = audio_callback;
  int ret = SDL_InitSubSystem(SDL_INIT_AUDIO);
  if (ret == 0) {
    SDL_OpenAudio(&s, NULL);
    SDL_PauseAudio(0);
  } else {
    printf("ret = %d\n", ret);
  }
  printf("freq = %d channels = %d samples = %d\n", freq, channels, samples);
}


static void audio_io_handler(uint32_t offset, int len, bool is_write) {
  uint32_t size = sizeof(uint32_t);
  assert((offset % 4 == 0) && (len == 4));
  uint32_t reg_index = offset / size;
  if(!is_write){
    switch (reg_index) {
      case reg_sbuf_size:
        audio_base[reg_sbuf_size] = sbuf_size;
        break;
      case reg_count:
        audio_base[reg_count] = sbuf_right - sbuf_left;
        break;
      case reg_sbuf_right:
        audio_base[reg_sbuf_right] = sbuf_right;
        break;
      case reg_sbuf_left:
        audio_base[reg_sbuf_left] = sbuf_left;
        break;
      default:
        panic("unknown behavior in audio_io_handler. is_write = %s, offset = %x", is_write ? "true" : "false", offset);
        break;
    }
  } else {
    switch (reg_index) {
      case reg_freq:
        freq = audio_base[reg_freq];
        break;
      case reg_channels:
        channels = audio_base[reg_channels];
        break;
      case reg_samples:
        samples = audio_base[reg_samples];
        break;
      case reg_init:
        uint32_t init = audio_base[reg_init];
        if ( init ) {
          audio_init(freq, channels, samples);
        }
        break;
      case reg_sbuf_right:
        sbuf_right = audio_base[reg_sbuf_right];
        break;
      // case reg_sbuf_left:
      //   sbuf_left = audio_base[reg_sbuf_left];
        break;
      default:
        panic("unknown behavior in audio_io_handler. is_write = %s, offset = %x", is_write ? "true" : "false", offset);
        break;
    }
  }
}

void init_audio() {
  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  audio_base = (uint32_t *)new_space(space_size);
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#endif

  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);
}
