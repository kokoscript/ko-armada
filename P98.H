int init_get_fb_location();
int init_pegc_dpmi(int fb_location);

inline void pegc_push(unsigned char* buffer);
inline void pegc_vsync();
void pegc_gfx_on();
void pegc_gfx_off();
void pegc_text_on();
void pegc_text_off();

inline void pegc_col_set_sep(unsigned char idx, unsigned char r, unsigned char g, unsigned char b);
inline void pegc_col_set(unsigned char idx, unsigned int col);
void pegc_pal_set(const unsigned int* pal, unsigned char size);

void pegc_setmode();
int pegc_start(int retain_text);
void pegc_stop();

inline void buz_on(unsigned char tone);
inline void buz_off();
