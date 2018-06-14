/* See LICENSE for licence details. */
/* yaft.h: define const/struct/global */
#ifndef _YAFT_H_
#define _YAFT_H_

#define _XOPEN_SOURCE 600
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

#include "glyph.h"
#include "color.h"

enum char_code {
	/* 7 bit */
	BEL = 0x07, BS  = 0x08, HT  = 0x09,
	LF  = 0x0A, VT  = 0x0B, FF  = 0x0C,
	CR  = 0x0D, ESC = 0x1B, DEL = 0x7F,
	/* others */
	SPACE     = 0x20,
	BACKSLASH = 0x5C,
};

enum misc {
	BUFSIZE            = 1024,             /* read, esc, various buffer size */
	BITS_PER_BYTE      = 8,                /* bits per byte */
	BITS_PER_PIXEL     = BITS_PER_BYTE * 4, /* RGBA: each 1 byte */
	ESCSEQ_SIZE        = 1024,             /* limit size of terminal escape sequence */
	SELECT_TIMEOUT     = 15000,            /* micro seconds: used by select() */
	SLEEP_TIME         = 30000,            /* sleep time at EAGAIN, EWOULDBLOCK (usec) */
	MAX_ARGS           = 16,               /* max parameters of csi/osc sequence */
	UCS2_CHARS         = 0x10000,          /* number of UCS2 glyphs */
	CTRL_CHARS         = 0x20,             /* number of ctrl_func */
	ESC_CHARS          = 0x80,             /* number of esc_func */
	DEFAULT_CHAR       = SPACE,            /* used for erase char */
	BRIGHT_INC         = 8,                /* value used for brightening color */
};

enum char_attr {
	ATTR_RESET     = 0,
	ATTR_BOLD      = 1, /* brighten foreground */
	ATTR_UNDERLINE = 4,
	ATTR_BLINK     = 5, /* brighten background */
	ATTR_REVERSE   = 7,
};

enum osc {
	OSC_GWREPT = 8900, /* OSC Ps: mode number of yaft GWREPT */
};

enum term_mode {
	MODE_RESET   = 0x00,
	MODE_ORIGIN  = 0x01, /* origin mode: DECOM */
	MODE_CURSOR  = 0x02, /* cursor visible: DECTCEM */
	MODE_AMRIGHT = 0x04, /* auto wrap: DECAWM */
	MODE_VWBS    = 0x08, /* variable-width backspace */
};

enum esc_state {
	STATE_RESET = 0x00,
	STATE_ESC   = 0x01, /* 0x1B, \033, ESC */
	STATE_CSI   = 0x02, /* ESC [ */
	STATE_OSC   = 0x04, /* ESC ] */
	STATE_DCS   = 0x08, /* ESC P */
};

enum glyph_width {
	NEXT_TO_WIDE = 0,
	HALF,
	WIDE,
};

enum loglevel_t {
	 LOG_DEBUG = 0,
	 LOG_WARN,
	 LOG_ERROR,
	 LOG_FATAL,
};

struct margin_t { uint16_t top, bottom; };
struct point_t { uint16_t x, y; };
struct color_pair_t { uint8_t fg, bg; };

struct cell_t {
	const struct glyph_t *glyphp;   /* pointer to glyph */
	struct color_pair_t color_pair; /* color (fg, bg) */
	enum char_attr attribute;       /* bold, underscore, etc... */
	enum glyph_width width;         /* wide char flag: WIDE, NEXT_TO_WIDE, HALF */
};

struct esc_t {
	char *buf;
	char *bp;
	int size;
	enum esc_state state;
};

struct charset_t {
	uint32_t code;     /* UCS2 code point: yaft only supports BMP */
	int following_byte, count;
	bool is_valid;
};

struct state_t {   /* for save, restore state */
	struct point_t cursor;
	enum term_mode mode;
	enum char_attr attribute;
};

struct terminal_t {
	int fd;                                  /* master of pseudo terminal */
	int width, height;                       /* terminal size (pixel) */
	int cols, lines;                         /* terminal size (cell) */
	struct cell_t **cells;                   /* pointer to each cell: cells[y * lines + x] */
	struct margin_t scroll;                  /* scroll margin */
	struct point_t cursor;                   /* cursor pos (x, y) */
	bool *line_dirty;                        /* dirty flag */
	bool *tabstop;                           /* tabstop flag */
	enum term_mode mode;                     /* for set/reset mode */
	bool wrap_occured;                       /* whether auto wrap occured or not */
	struct state_t state;                    /* for restore */
	struct color_pair_t color_pair;          /* color (fg, bg) */
	enum char_attr attribute;                /* bold, underscore, etc... */
	struct charset_t charset;                /* store UTF-8 byte stream */
	struct esc_t esc;                        /* store escape sequence */
	const struct glyph_t *glyph[UCS2_CHARS]; /* array of pointer to glyphs[] */
};

struct fb_info_t {
	struct bitfield_t {
		int length;
		int offset;
	} red, green, blue, alpha;
	int width, height;       /* display resolution */
	long screen_size;        /* screen data size (byte) */
	int line_length;         /* line length (byte) */
	int bytes_per_pixel;
	int bits_per_pixel;
};

struct framebuffer_t {
	uint8_t *buf;                   /* copy of framebuffer */
	uint32_t real_palette[NCOLORS]; /* hardware specific color palette */
	struct fb_info_t info;
};

struct parm_t { /* for parse_arg() */
	int argc;
	char *argv[MAX_ARGS];
};

extern const uint8_t attr_mask[];
extern const uint32_t bit_mask[];

extern volatile sig_atomic_t child_alive; /* SIGCHLD: child process (shell) is alive or not */
extern struct framebuffer_t fb;
extern struct terminal_t term;

/* yaft.c: include main function */
void sig_handler(int signo);
void set_rawmode(int fd, struct termios *save_tm);
bool signal_init(void);
void signal_die(void);
bool fork_and_exec(int *master, int lines, int cols);
int check_fds(fd_set *fds, struct timespec *ts, int master);
bool c_init(int width, int height);
bool c_select(void);
void c_write(const char *str, size_t size);

#endif /* _YAFT_H_ */
