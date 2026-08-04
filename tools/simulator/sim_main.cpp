/**
 ******************************************************************************
 * @brief:  Bản chạy thử Dungeon trên máy tính.
 *
 *  Hai kiểu hiển thị, cùng một firmware bên dưới:
 *
 *    ./dungeon-sim          -> vẽ thẳng trong Terminal bằng ký tự nửa khối
 *    ./dungeon-sim --web    -> mở http://localhost:8080, nhìn như cái máy thật
 *
 *  Không cần thư viện ngoài. Chỉ dùng POSIX nên biên dịch được bằng clang có
 *  sẵn trên macOS, không phải cài thêm gì.
 ******************************************************************************
**/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sim.h"

extern "C" {
#include "ak.h"
#include "task.h"
}

void sim_boot();
void sim_tick_1ms();

#define LCD_W 128
#define LCD_H 64

static volatile sig_atomic_t g_quit = 0;
static void on_sigint(int) { g_quit = 1; }

/*****************************************************************************/
/*  Đồng hồ thật của máy
 */
/*****************************************************************************/
static uint64_t now_us() {
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

/* Chạy firmware cho kịp thời gian thật. Mỗi vòng chỉ bù tối đa 50 ms để nếu
 * cửa sổ bị treo lâu thì không phải chạy bù hàng nghìn tick một lúc. */
static void catch_up(uint64_t* last_us) {
	uint64_t now = now_us();
	uint64_t due = (now - *last_us) / 1000;
	if (due > 50) { due = 50; *last_us = now; }
	else { *last_us += due * 1000; }
	for (uint64_t i = 0; i < due; i++) { sim_tick_1ms(); }
}

/*****************************************************************************/
/*  Kiểu 1 - vẽ trong Terminal
 *
 *  Ghép hai dòng pixel vào một dòng ký tự bằng nửa khối trên / dưới, nên
 *  màn 128x64 vừa đúng 128 cột x 32 dòng, không méo tỉ lệ.
 */
/*****************************************************************************/
static struct termios g_tio_saved;
static int g_tio_active = 0;

static void term_raw_on() {
	if (!isatty(STDIN_FILENO)) { return; }
	tcgetattr(STDIN_FILENO, &g_tio_saved);
	struct termios t = g_tio_saved;
	t.c_lflag &= ~(ICANON | ECHO);
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	g_tio_active = 1;
	printf("\033[?25l");            /* giấu con trỏ */
	fflush(stdout);
}

static void term_raw_off() {
	if (!g_tio_active) { return; }
	tcsetattr(STDIN_FILENO, TCSANOW, &g_tio_saved);
	printf("\033[?25h\033[0m\n");   /* hiện lại con trỏ */
	fflush(stdout);
	g_tio_active = 0;
}

static void term_draw() {
	const uint8_t* fb = sim_framebuffer();
	static char out[64 * 1024];
	int n = 0;

	n += snprintf(out + n, sizeof(out) - n, "\033[H");   /* về góc trên trái */

	n += snprintf(out + n, sizeof(out) - n, "\033[38;5;240m  +");
	for (int i = 0; i < LCD_W; i++) { n += snprintf(out + n, sizeof(out) - n, "-"); }
	n += snprintf(out + n, sizeof(out) - n, "+\033[0m\n");

	for (int y = 0; y < LCD_H; y += 2) {
		n += snprintf(out + n, sizeof(out) - n, "\033[38;5;240m  |\033[0m");
		for (int x = 0; x < LCD_W; x++) {
			int top = fb[y * LCD_W + x];
			int bot = fb[(y + 1) * LCD_W + x];
			const char* g = " ";
			if (top && bot)      { g = "█"; }
			else if (top)        { g = "▀"; }
			else if (bot)        { g = "▄"; }
			n += snprintf(out + n, sizeof(out) - n, "\033[38;5;194m%s", g);
		}
		n += snprintf(out + n, sizeof(out) - n, "\033[38;5;240m|\033[0m\n");
	}

	n += snprintf(out + n, sizeof(out) - n, "\033[38;5;240m  +");
	for (int i = 0; i < LCD_W; i++) { n += snprintf(out + n, sizeof(out) - n, "-"); }
	n += snprintf(out + n, sizeof(out) - n, "+\033[0m\n\n");

	n += snprintf(out + n, sizeof(out) - n, "  \033[38;5;245m%s\033[0m\033[K\n", sim_status_line());
	n += snprintf(out + n, sizeof(out) - n,
	              "  \033[38;5;245mUp/Down hoac W/S = nut UP/DOWN   Enter hoac Space = nut MODE\033[0m\033[K\n"
	              "  \033[38;5;245mShift+W / Shift+S = bam giu      R = Reset      Q = thoat\033[0m\033[K\n");

	fwrite(out, 1, n, stdout);
	fflush(stdout);
}

static void term_input() {
	unsigned char c;
	while (read(STDIN_FILENO, &c, 1) == 1) {
		if (c == 0x1B) {                       /* phím mũi tên: ESC [ A/B */
			unsigned char a, b;
			if (read(STDIN_FILENO, &a, 1) != 1) { continue; }
			if (read(STDIN_FILENO, &b, 1) != 1) { continue; }
			if (a == '[' && b == 'A') { sim_press(SIM_BTN_UP); }
			if (a == '[' && b == 'B') { sim_press(SIM_BTN_DOWN); }
			continue;
		}
		switch (c) {
		case 'w': case 'k': sim_press(SIM_BTN_UP);        break;
		case 's': case 'j': sim_press(SIM_BTN_DOWN);      break;
		case 'W': case 'K': sim_press(SIM_BTN_UP_LONG);   break;
		case 'S': case 'J': sim_press(SIM_BTN_DOWN_LONG); break;
		case '\r': case '\n': case ' ': sim_press(SIM_BTN_MODE); break;
		case 'm': sim_press(SIM_BTN_MODE);       break;
		case 'M': sim_press(SIM_BTN_MODE_LONG);  break;
		case 'r': case 'R': sim_press(SIM_BTN_RESET); break;
		case 'q': case 'Q': case 3: g_quit = 1;  break;
		default: break;
		}
	}
}

static void term_poll() {
	static uint64_t last = 0, last_draw = 0;
	if (!last) { last = now_us(); printf("\033[2J"); }

	catch_up(&last);
	term_input();

	uint64_t now = now_us();
	if (now - last_draw > 33000) { last_draw = now; term_draw(); }
	usleep(1500);
}

/*****************************************************************************/
/*  Kiểu 2 - phục vụ một trang web
 *
 *  Máy chủ HTTP tối giản, một luồng, không chặn. Mỗi vòng lặp xử lý đúng một
 *  yêu cầu rồi quay lại chạy firmware, nên game không bị khựng.
 */
/*****************************************************************************/
static const char* PAGE =
"<!doctype html><html><head><meta charset=utf-8><title>Dungeon - simulator</title>"
"<style>"
"*{box-sizing:border-box}"
"body{margin:0;height:100vh;display:flex;align-items:center;justify-content:center;"
"background:#111014;color:#c9c6bd;font:14px ui-monospace,SFMono-Regular,Menlo,monospace}"
".wrap{text-align:center}"
".dev{background:#26242c;border:2px solid #3d3a46;border-radius:18px;padding:22px;"
"box-shadow:0 24px 60px rgba(0,0,0,.6)}"
".scr{background:#050705;border-radius:8px;padding:10px;line-height:0}"
"canvas{width:768px;height:384px;image-rendering:pixelated;display:block}"
".pad{margin-top:16px;display:flex;gap:10px;justify-content:center}"
"button{font:inherit;background:#32303a;color:#d8d5cc;border:1px solid #4a4754;"
"border-radius:8px;padding:9px 18px;cursor:pointer}"
"button:active{transform:translateY(1px);background:#3d3a46}"
".st{margin-top:14px;font-size:12px;color:#8a877f;min-height:1.2em}"
".hint{margin-top:6px;font-size:12px;color:#6c6960}"
"</style></head><body><div class=wrap>"
"<div class=dev><div class=scr><canvas id=c width=128 height=64></canvas></div>"
"<div class=pad>"
"<button onclick=\"k('up')\">UP</button>"
"<button onclick=\"k('mode')\">MODE</button>"
"<button onclick=\"k('down')\">DOWN</button>"
"<button onclick=\"k('reset')\">RESET</button>"
"</div></div>"
"<div class=st id=s>dang ket noi...</div>"
"<div class=hint>Phim mui ten = UP/DOWN &nbsp;|&nbsp; Enter hoac Space = MODE &nbsp;|&nbsp; "
"Shift+mui ten = bam giu &nbsp;|&nbsp; R = Reset</div>"
"</div>"
"<script>"
"var cv=document.getElementById('c'),cx=cv.getContext('2d');"
"var img=cx.createImageData(128,64);"
"function k(n){fetch('/key?k='+n)}"
"function draw(hex){var d=img.data;"
"for(var i=0;i<8192;i++){var on=parseInt(hex.substr(i>>2,1),16)>>(3-(i&3))&1;"
"var o=i*4;d[o]=on?222:6;d[o+1]=on?236:9;d[o+2]=on?206:6;d[o+3]=255}"
"cx.putImageData(img,0,0)}"
"function poll(){fetch('/fb').then(function(r){return r.json()}).then(function(j){"
"draw(j.fb);document.getElementById('s').textContent=j.st})"
".catch(function(){document.getElementById('s').textContent='mat ket noi - simulator da tat?'})}"
"setInterval(poll,40);poll();"
"document.addEventListener('keydown',function(e){"
"var s=e.shiftKey;"
"if(e.key=='ArrowUp'){k(s?'uplong':'up');e.preventDefault()}"
"else if(e.key=='ArrowDown'){k(s?'downlong':'down');e.preventDefault()}"
"else if(e.key=='Enter'||e.key==' '){k(s?'modelong':'mode');e.preventDefault()}"
"else if(e.key=='r'||e.key=='R'){k('reset')}"
"});"
"</script></body></html>";

static void http_send(int fd, const char* ctype, const char* body, int len) {
	char hdr[256];
	int n = snprintf(hdr, sizeof(hdr),
	                 "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n"
	                 "Cache-Control: no-store\r\nConnection: close\r\n\r\n", ctype, len);
	if (write(fd, hdr, n) < 0) { return; }
	if (write(fd, body, len) < 0) { return; }
}

/* 8192 pixel -> 2048 ký tự hex, mỗi ký tự gói 4 pixel. */
static void pack_fb(char* out) {
	const uint8_t* fb = sim_framebuffer();
	static const char* H = "0123456789abcdef";
	for (int i = 0; i < LCD_W * LCD_H / 4; i++) {
		int v = (fb[i * 4 + 0] ? 8 : 0) | (fb[i * 4 + 1] ? 4 : 0) |
		        (fb[i * 4 + 2] ? 2 : 0) | (fb[i * 4 + 3] ? 1 : 0);
		out[i] = H[v];
	}
	out[LCD_W * LCD_H / 4] = 0;
}

static void handle_key(const char* q) {
	if      (strstr(q, "k=uplong"))   { sim_press(SIM_BTN_UP_LONG); }
	else if (strstr(q, "k=downlong")) { sim_press(SIM_BTN_DOWN_LONG); }
	else if (strstr(q, "k=modelong")) { sim_press(SIM_BTN_MODE_LONG); }
	else if (strstr(q, "k=up"))       { sim_press(SIM_BTN_UP); }
	else if (strstr(q, "k=down"))     { sim_press(SIM_BTN_DOWN); }
	else if (strstr(q, "k=mode"))     { sim_press(SIM_BTN_MODE); }
	else if (strstr(q, "k=reset"))    { sim_press(SIM_BTN_RESET); }
}

static int g_srv = -1;

static int web_listen(int port) {
	signal(SIGPIPE, SIG_IGN);

	int srv = socket(AF_INET, SOCK_STREAM, 0);
	if (srv < 0) { perror("socket"); return -1; }
	int on = 1;
	setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons((uint16_t)port);

	if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		fprintf(stderr, "[sim] khong bind duoc cong %d - dang co gi chay o do?\n", port);
		fprintf(stderr, "      thu:  ./dungeon-sim --web --port 8081\n");
		close(srv);
		return -1;
	}
	listen(srv, 8);
	fcntl(srv, F_SETFL, O_NONBLOCK);

	printf("\n  Dungeon simulator dang chay\n");
	printf("  Mo trinh duyet:  http://localhost:%d\n", port);
	printf("  Ctrl+C de dung\n\n");
	return srv;
}

static void web_poll() {
	static uint64_t last = 0;
	static char fbhex[LCD_W * LCD_H / 4 + 1];
	static char json[4096];
	if (!last) { last = now_us(); }

	catch_up(&last);

	int fd = accept(g_srv, 0, 0);
	if (fd >= 0) {
		char req[2048];
		ssize_t n = read(fd, req, sizeof(req) - 1);
		if (n > 0) {
			req[n] = 0;
			if (strncmp(req, "GET /fb", 7) == 0) {
				pack_fb(fbhex);
				int len = snprintf(json, sizeof(json), "{\"fb\":\"%s\",\"st\":\"%s\"}",
				                   fbhex, sim_status_line());
				http_send(fd, "application/json", json, len);
			}
			else if (strncmp(req, "GET /key", 8) == 0) {
				handle_key(req);
				http_send(fd, "text/plain", "ok", 2);
			}
			else {
				http_send(fd, "text/html; charset=utf-8", PAGE, (int)strlen(PAGE));
			}
		}
		close(fd);
	}
	else {
		usleep(1200);
	}
}

/*****************************************************************************/
/*  Nhịp của bản giả lập
 *
 *  Kernel AK gọi hàm này mỗi vòng của task_run(), thông qua polling task.
 *  Nghĩa là firmware vẫn tự chạy vòng lặp của nó y như trên bo, bản giả lập
 *  chỉ ké vào đó chứ không dựng vòng lặp riêng.
 */
/*****************************************************************************/
static int g_web_mode = 0;

void sim_poll() {
	if (g_quit) {
		if (!g_web_mode) { term_raw_off(); }
		printf("\n[sim] thoat sau %u ms firmware, %ld lan ghi EEPROM\n",
		       (unsigned)sim_ms, sim_ee_writes);
		exit(0);
	}
	if (g_web_mode) { web_poll(); }
	else            { term_poll(); }
}

/*****************************************************************************/
int main(int argc, char** argv) {
	int port = 8080, wipe = 0;
	const char* ee = "dungeon-sim.eeprom";

	for (int i = 1; i < argc; i++) {
		if      (strcmp(argv[i], "--web") == 0)   { g_web_mode = 1; }
		else if (strcmp(argv[i], "--wipe") == 0)  { wipe = 1; }
		else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) { port = atoi(argv[++i]); }
		else if (strcmp(argv[i], "--eeprom") == 0 && i + 1 < argc) { ee = argv[++i]; }
		else {
			printf("Cach dung: %s [--web] [--port N] [--wipe] [--eeprom FILE]\n", argv[0]);
			printf("  (khong tham so)  ve thang trong Terminal\n");
			printf("  --web            mo http://localhost:8080\n");
			printf("  --wipe           xoa EEPROM truoc khi chay (mat save, mat diem)\n");
			return 0;
		}
	}

	signal(SIGINT, on_sigint);
	signal(SIGTERM, on_sigint);

	sim_eeprom_open(ee);
	if (wipe) { sim_eeprom_wipe(); }

	if (g_web_mode) {
		g_srv = web_listen(port);
		if (g_srv < 0) { return 1; }
	}
	else {
		term_raw_on();
		atexit(term_raw_off);
	}

	sim_boot();

	/* Vòng lặp là của kernel, không phải của bản giả lập: task_run() chạy
	 * task_sheduler() rồi task_polling_run() mãi mãi, và task_polling_run()
	 * gọi sim_poll(). Hàm này không bao giờ trả về. */
	return task_run();
}
