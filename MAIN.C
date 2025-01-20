#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <pc.h>
#include "P98.H"
#include "PEGC.H"
#include "DATUM.H"

#define INSTANCE_POOL_SIZE 16
#define ALT_INV_AFFINITY 10
#define SPAWN_INV_AFFINITY 80

typedef struct KoInstance {
	int anim_set;
	int x, y;
	int frame;
	int elapsed;
	int step;
} KoInstance;

KoInstance* inst_pool[INSTANCE_POOL_SIZE];
unsigned char* buf;

KoInstance* new_ko() {
	KoInstance* inst = (KoInstance*) malloc(sizeof(KoInstance));
	if (rand() % ALT_INV_AFFINITY > 0)
		inst->anim_set = 0;
	else
		inst->anim_set = 1;
	inst->x = -QUILT_SET1_W;
	inst->y = rand() % (PEGC_SIZE_H - QUILT_SET1_H);
	inst->frame = 0;
	inst->elapsed = 0;
	inst->step = 0;
	return inst;
}

void blit2buf(unsigned int src_x, unsigned int src_y, unsigned int src_w, unsigned int src_h, int dest_x, int dest_y) {
	int x, y;
	int start_x = 0;
	int end_x = src_w;
	int start_y = 0;
	int end_y = src_h;
	unsigned char srcpx;

	if (dest_x < 0)
		start_x = -dest_x;
	else if (dest_x + src_w >= PEGC_SIZE_W)
		end_x = PEGC_SIZE_W - dest_x;
	if (dest_y < 0)
		start_y = -dest_y;
	else if (dest_y + src_h >= PEGC_SIZE_H)
		end_y = PEGC_SIZE_H - dest_y;
	
	for (y = start_y; y < end_y; y++) {
		for (x = start_x; x < end_x; x++) {
			srcpx = quilt[(QUILT_W * (y + src_y)) + x + src_x];
			if (srcpx)
				buf[(PEGC_SIZE_W * (y + dest_y)) + x + dest_x] = srcpx;
		}
	}
}

int find_slot() {
	int i;
	for (i = 0; i < INSTANCE_POOL_SIZE; i++) {
		if (inst_pool[i] == NULL)
			return i;
	}
	return -1;
}

inline void update_ko(KoInstance* inst, int origin) {
	if (inst->x > PEGC_SIZE_W) {
		free(inst_pool[origin]);
		inst_pool[origin] = NULL;
		return;
	}

	inst->x++;
	inst->elapsed++;
	if (inst->elapsed > QUILT_ANIM_TIME) {
		inst->elapsed = 0;
		inst->frame++;
		if (inst->frame >= QUILT_ANIM_FRAMES) {
			inst->frame = 0;
			if (inst->anim_set == 0) {
				// this keeps playing the same tone if only 1 instance is active... not sure why
				if (inst->step == 1) {
					inst->step = 0;
					buz_on(12);
				} else {
					inst->step = 1;
					buz_on(6);
				}
			}
		}
	}
}

inline void update() {
	int new_idx, i;
	
	if (rand() % SPAWN_INV_AFFINITY == 0) {
		new_idx = find_slot();
		if (new_idx > -1)
			inst_pool[new_idx] = new_ko();
	}

	for (i = 0; i < INSTANCE_POOL_SIZE; i++) {
		if (inst_pool[i] != NULL)
			update_ko(inst_pool[i], i);
	}
}

inline void draw_ko(KoInstance* inst) {
	int quilt_x, quilt_y;
	int spr_w, spr_h;
	if (inst->anim_set == 0) {
		quilt_x = inst->frame * QUILT_SET1_W;
		quilt_y = 0;
		spr_w = QUILT_SET1_W;
		spr_h = QUILT_SET1_H;
	} else {
		quilt_x = inst->frame * QUILT_SET2_W;
		quilt_y = QUILT_SET1_H;
		spr_w = QUILT_SET2_W;
		spr_h = QUILT_SET2_H;
	}
	blit2buf(quilt_x, quilt_y, spr_w, spr_h, inst->x, inst->y);
}

inline void draw() {
	int i;
	memset(buf, 1, PEGC_SIZE);
	for (i = 0; i < INSTANCE_POOL_SIZE; i++) {
		if (inst_pool[i] != NULL)
			draw_ko(inst_pool[i]);
	}
}

int main() {
	int i;
	buf = (unsigned char*) malloc(PEGC_SIZE_W * PEGC_SIZE_H);
	memset(inst_pool, NULL, INSTANCE_POOL_SIZE);

	if (!pegc_start(1))
		return 1;

	pegc_pal_set(pal, PAL_SIZE);

	while (!kbhit()) {
		buz_off();
		update();
		draw();
		pegc_vsync();
		pegc_push(buf);
	}
	
	for (i = 0; i < INSTANCE_POOL_SIZE; i++) {
		if (inst_pool[i] != NULL)
			free(inst_pool[i]);
	}
	buz_off();
	pegc_stop();
	return 0;
}
