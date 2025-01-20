#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <dos.h>
#include <sys/farptr.h>
#include <sys/movedata.h>
#include <go32.h>
#include <dpmi.h>
#include "P98.H"
#include "PEGC.H"
#include "BUZZER.H"

#define RETURN_OK (1)
#define RETURN_ERR (0)

__dpmi_meminfo pegc_dpmi;
int pegc_sel = -1;

int init_get_fb_location() {
	int status = inportb(MEMHOLE_STAT_ADDR);
	if (status & 0x04){
		printf("No hole... VRAM FB @ 0x%x\n", PEGC_FB_LOCATION_HIGH);
		return PEGC_FB_LOCATION_HIGH;
	} else {
		printf("Hole! VRAM FB @ 0x%x\n", PEGC_FB_LOCATION_LOW);
		return PEGC_FB_LOCATION_LOW;
	}
}

int init_pegc_dpmi(int fb_location) {
	if (!__djgpp_nearptr_enable()) {
		printf("DPMI: failed to enable nearptrs");
		return RETURN_ERR;
	}

	pegc_dpmi.address = fb_location;
	pegc_dpmi.size = PEGC_FB_SIZE;
	
	if (__dpmi_physical_address_mapping(&pegc_dpmi)) {
		printf("DPMI: couldn't map PEGC\n");
		return RETURN_ERR;
	}

	pegc_sel = __dpmi_allocate_ldt_descriptors(1);
	if (pegc_sel < 0){
		printf("DPMI: can't allocate PEGC descriptor\n");
		__dpmi_free_physical_address_mapping(&pegc_dpmi);
		return RETURN_ERR;
	}
	__dpmi_set_segment_base_address(pegc_sel, pegc_dpmi.address);
	__dpmi_set_segment_limit(pegc_sel, pegc_dpmi.size - 1);

	printf("DPMI: mapped PEGC FB!\n");
	return RETURN_OK;
}

inline void pegc_push(unsigned char* buffer) {
	movedata(_my_ds(), buffer, pegc_sel, 0, PEGC_SIZE);
}

inline void pegc_vsync() {
	while ((inportb(PEGC_STAT_ADDR) & PEGC_STAT_VSYNC) == 0);
}

void pegc_gfx_on() {
	outportb(PEGC_CMD_ADDR, PEGC_CMD_START);
}

void pegc_gfx_off() {
	outportb(PEGC_CMD_ADDR, PEGC_CMD_STOP1);
}

void pegc_text_on() {
	outportb(PEGC_TEXT_CMD_ADDR, PEGC_CMD_START);
}

void pegc_text_off() {
	outportb(PEGC_TEXT_CMD_ADDR, PEGC_CMD_STOP1);
}

inline void pegc_col_set_sep(unsigned char idx, unsigned char r, unsigned char g, unsigned char b) {
	outportb(PEGC_PAL_SEL_ADDR, idx);
	outportb(PEGC_PAL_R_ADDR, r);
	outportb(PEGC_PAL_G_ADDR, g);
	outportb(PEGC_PAL_B_ADDR, b);
}

inline void pegc_col_set(unsigned char idx, unsigned int col) {
	outportb(PEGC_PAL_SEL_ADDR, idx);
	outportb(PEGC_PAL_R_ADDR, (col >> 16));
	outportb(PEGC_PAL_G_ADDR, (col >> 8));
	outportb(PEGC_PAL_B_ADDR, col);
}

void pegc_pal_set(const unsigned int* pal, unsigned char size) {
	int i;
	for (i = 0; i < size; i++)
		pegc_col_set(i, pal[i]);
}

void pegc_setmode() {
	outportb(PEGC_SCANFREQ_ADDR, PEGC_SCANFREQ_31);
	outportb(PEGC_DEPTH_ADDR, PEGC_DEPTH_UNLOCK);
	outportb(PEGC_DEPTH_ADDR, PEGC_DEPTH_8);
	outportb(PEGC_DEPTH_ADDR, PEGC_DEPTH_PAGES_1);
	outportb(PEGC_DEPTH_ADDR, PEGC_DEPTH_LOCK);
	_farpokeb(_dos_ds, PEGC_FORMAT_ADDR, PEGC_FORMAT_LINEAR);
}

int pegc_start(int retain_text) {
	pegc_setmode();

	// setup dpmi mapping
	if (!init_pegc_dpmi(init_get_fb_location())) {
		pegc_stop();
		return RETURN_ERR;
	}

	if (!retain_text)
		pegc_text_off();

	// enable framebuffer, select+write page 0, go.
	_farpokeb(_dos_ds, PEGC_FB_CTRL_ADDR, PEGC_FB_ON);
	outportb(PEGC_DRAW_PAGE_SEL_ADDR, PEGC_PAGE0);
	outportb(PEGC_DISP_PAGE_SEL_ADDR, PEGC_PAGE0);
	pegc_gfx_on();
	return RETURN_OK;
}

void pegc_stop() {
	_farpokeb(_dos_ds, PEGC_FB_CTRL_ADDR, PEGC_FB_OFF);

	// return to 16 color mode, disable graphics
	outportb(PEGC_DEPTH_ADDR, PEGC_DEPTH_UNLOCK);
	outportb(PEGC_DEPTH_ADDR, PEGC_DEPTH_4);
	outportb(PEGC_DEPTH_ADDR, PEGC_DEPTH_LOCK);
	pegc_gfx_off();

	// remove mapping
	__dpmi_free_physical_address_mapping(&pegc_dpmi);

	pegc_text_on();
}

inline void buz_on(unsigned char tone) {
	unsigned char buz_status = inportb(BUZ_ADDR);
	outportb(BUZ_ADDR, buz_status & BUZ_BIT_ON);
	outportb(BUZ_TONE_ADDR, tone);
}

inline void buz_off() {
	unsigned char buz_status = inportb(BUZ_ADDR);
	outportb(BUZ_ADDR, buz_status | BUZ_BIT_OFF);
}
