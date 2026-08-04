#ifndef __ADAFRUIT_GFX_H__
#define __ADAFRUIT_GFX_H__
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pgmspace.h"

/* font 5x7 thật, trích từ driver/Adafruit_oled_drv/glcdfont.cpp */
static const unsigned char GLCD_FONT[1275] = {0,0,0,0,0,62,91,79,91,62,62,107,79,107,62,28,62,124,62,28,24,60,126,60,24,28,87,125,87,28,28,94,127,94,28,0,24,60,24,0,255,231,195,231,255,0,24,36,24,0,255,231,219,231,255,48,72,58,6,14,38,41,121,41,38,64,127,5,5,7,64,127,5,37,63,90,60,231,60,90,127,62,28,28,8,8,28,28,62,127,20,34,127,34,20,95,95,0,95,95,6,9,127,1,127,0,102,137,149,106,96,96,96,96,96,148,162,255,162,148,8,4,126,4,8,16,32,126,32,16,8,8,42,28,8,8,28,42,8,8,30,16,16,16,16,12,30,12,30,12,48,56,62,56,48,6,14,62,14,6,0,0,0,0,0,0,0,95,0,0,0,7,0,7,0,20,127,20,127,20,36,42,127,42,18,35,19,8,100,98,54,73,86,32,80,0,8,7,3,0,0,28,34,65,0,0,65,34,28,0,42,28,127,28,42,8,8,62,8,8,0,128,112,48,0,8,8,8,8,8,0,0,96,96,0,32,16,8,4,2,62,81,73,69,62,0,66,127,64,0,114,73,73,73,70,33,65,73,77,51,24,20,18,127,16,39,69,69,69,57,60,74,73,73,49,65,33,17,9,7,54,73,73,73,54,70,73,73,41,30,0,0,20,0,0,0,64,52,0,0,0,8,20,34,65,20,20,20,20,20,0,65,34,20,8,2,1,89,9,6,62,65,93,89,78,124,18,17,18,124,127,73,73,73,54,62,65,65,65,34,127,65,65,65,62,127,73,73,73,65,127,9,9,9,1,62,65,65,81,115,127,8,8,8,127,0,65,127,65,0,32,64,65,63,1,127,8,20,34,65,127,64,64,64,64,127,2,28,2,127,127,4,8,16,127,62,65,65,65,62,127,9,9,9,6,62,65,81,33,94,127,9,25,41,70,38,73,73,73,50,3,1,127,1,3,63,64,64,64,63,31,32,64,32,31,63,64,56,64,63,99,20,8,20,99,3,4,120,4,3,97,89,73,77,67,0,127,65,65,65,2,4,8,16,32,0,65,65,65,127,4,2,1,2,4,64,64,64,64,64,0,3,7,8,0,32,84,84,120,64,127,40,68,68,56,56,68,68,68,40,56,68,68,40,127,56,84,84,84,24,0,8,126,9,2,24,164,164,156,120,127,8,4,4,120,0,68,125,64,0,32,64,64,61,0,127,16,40,68,0,0,65,127,64,0,124,4,120,4,120,124,8,4,4,120,56,68,68,68,56,252,24,36,36,24,24,36,36,24,252,124,8,4,4,8,72,84,84,84,36,4,4,63,68,36,60,64,64,32,124,28,32,64,32,28,60,64,48,64,60,68,40,16,40,68,76,144,144,144,124,68,100,84,76,68,0,8,54,65,0,0,0,119,0,0,0,65,54,8,0,2,1,2,4,2,60,38,35,38,60,30,161,161,97,18,58,64,64,32,122,56,84,84,85,89,33,85,85,121,65,33,84,84,120,65,33,85,84,120,64,32,84,85,121,64,12,30,82,114,18,57,85,85,85,89,57,84,84,84,89,57,85,84,84,88,0,0,69,124,65,0,2,69,125,66,0,1,69,124,64,240,41,36,41,240,240,40,37,40,240,124,84,85,69,0,32,84,84,124,84,124,10,9,127,73,50,73,73,73,50,50,72,72,72,50,50,74,72,72,48,58,65,65,33,122,58,66,64,32,120,0,157,160,160,125,57,68,68,68,57,61,64,64,64,61,60,36,255,36,36,72,126,73,67,102,43,47,252,47,43,255,9,41,246,32,192,136,126,9,3,32,84,84,121,65,0,0,68,125,65,48,72,72,74,50,56,64,64,34,122,0,122,10,10,114,125,13,25,49,125,38,41,41,47,40,38,41,41,41,38,48,72,77,64,32,56,8,8,8,8,8,8,8,8,56,47,16,200,172,186,47,16,40,52,250,0,0,123,0,0,8,20,42,20,34,34,20,42,20,8,170,0,85,0,170,170,85,170,85,170,0,0,0,255,0,16,16,16,255,0,20,20,20,255,0,16,16,255,0,255,16,16,240,16,240,20,20,20,252,0,20,20,247,0,255,0,0,255,0,255,20,20,244,4,252,20,20,23,16,31,16,16,31,16,31,20,20,20,31,0,16,16,16,240,0,0,0,0,31,16,16,16,16,31,16,16,16,16,240,16,0,0,0,255,16,16,16,16,16,16,16,16,16,255,16,0,0,0,255,20,0,0,255,0,255,0,0,31,16,23,0,0,252,4,244,20,20,23,16,23,20,20,244,4,244,0,0,255,0,247,20,20,20,20,20,20,20,247,0,247,20,20,20,23,20,16,16,31,16,31,20,20,20,244,20,16,16,240,16,240,0,0,31,16,31,0,0,0,31,20,0,0,0,252,20,0,0,240,16,240,16,16,255,16,255,20,20,20,255,20,16,16,16,31,0,0,0,0,240,16,255,255,255,255,255,240,240,240,240,240,255,255,255,0,0,0,0,0,255,255,15,15,15,15,15,56,68,68,56,68,124,42,42,62,20,126,2,2,6,6,2,126,2,126,2,99,85,73,65,99,56,68,68,60,4,64,126,32,30,32,6,2,126,2,2,153,165,231,165,153,28,42,73,42,28,76,114,1,114,76,48,74,77,77,48,48,72,120,72,48,188,98,90,70,61,62,73,73,73,0,126,1,1,1,126,42,42,42,42,42,68,68,95,68,68,64,81,74,68,64,64,68,74,81,64,0,0,255,1,3,224,128,255,0,0,8,8,107,107,8,54,18,54,36,54,6,15,9,15,6,0,0,24,24,0,0,0,16,16,0,48,64,255,1,1,0,31,1,1,30,0,25,29,23,18,0,60,60,60,60,0,0,0,0,0};
#define BLACK 0
#define WHITE 1
/* Host build of Adafruit_GFX.
 *
 * The real driver in application/sources/driver/Adafruit_oled_drv/ drags in the
 * Arduino compatibility layer (Arduino.h -> sys_ctrl, sys_io, sys_cfg and
 * Print.h -> WString, Printable), none of which mean anything on a laptop.
 * So the drawing primitives are re-implemented here, byte-for-byte identical
 * to the originals: same Bresenham circle, same corner helpers, same 5x8 font,
 * same cursor advance of 6*size. What changes is only where the pixels land -
 * a plain linear framebuffer instead of the SSD1309 page layout. */
class Adafruit_GFX {
public:
	Adafruit_GFX(int16_t w=128, int16_t h=64) : WIDTH(w), HEIGHT(h) { memset(fb,0,sizeof(fb)); }
	void px(int16_t x,int16_t y,uint16_t c){ if(x<0||y<0||x>=WIDTH||y>=HEIGHT){oob++;return;} if(c)fb[y*128+x]=1; else fb[y*128+x]=0; }
	virtual void drawPixel(int16_t x,int16_t y,uint16_t c){ px(x,y,c); }
	void drawFastVLine(int16_t x,int16_t y,int16_t h,uint16_t c){ for(int16_t i=0;i<h;i++)px(x,y+i,c); }
	void drawFastHLine(int16_t x,int16_t y,int16_t w,uint16_t c){ for(int16_t i=0;i<w;i++)px(x+i,y,c); }
	void drawLine(int16_t x0,int16_t y0,int16_t x1,int16_t y1,uint16_t c){
		int dx = x1>x0? x1-x0 : x0-x1, sx = x0<x1? 1 : -1;
		int dy = y1>y0? y1-y0 : y0-y1, sy = y0<y1? 1 : -1;
		int err = (dx>dy? dx : -dy)/2;
		for(;;){ px(x0,y0,c); if(x0==x1&&y0==y1) break;
			int e2=err; if(e2>-dx){err-=dy;x0+=sx;} if(e2<dy){err+=dx;y0+=sy;} }
	}
	void drawRect(int16_t x,int16_t y,int16_t w,int16_t h,uint16_t c){ drawFastHLine(x,y,w,c); drawFastHLine(x,y+h-1,w,c); drawFastVLine(x,y,h,c); drawFastVLine(x+w-1,y,h,c); }
	void fillRect(int16_t x,int16_t y,int16_t w,int16_t h,uint16_t c){ for(int16_t j=0;j<h;j++) drawFastHLine(x,y+j,w,c); }
	void fillScreen(uint16_t c){ fillRect(0,0,WIDTH,HEIGHT,c); }
	/* Bốn góc bo: chép nguyên thuật toán Bresenham của Adafruit_GFX thật,
	 * vì màn Menu và màn Setting vẽ thẻ bằng mấy hàm này, sai một pixel là
	 * nhìn ra ngay. */
	void drawCircleHelper(int16_t x0,int16_t y0,int16_t r,uint8_t corner,uint16_t c){
		int16_t f=1-r, ddF_x=1, ddF_y=-2*r, x=0, y=r;
		while(x<y){
			if(f>=0){ y--; ddF_y+=2; f+=ddF_y; }
			x++; ddF_x+=2; f+=ddF_x;
			if(corner&0x4){ px(x0+x,y0+y,c); px(x0+y,y0+x,c); }
			if(corner&0x2){ px(x0+x,y0-y,c); px(x0+y,y0-x,c); }
			if(corner&0x8){ px(x0-y,y0+x,c); px(x0-x,y0+y,c); }
			if(corner&0x1){ px(x0-y,y0-x,c); px(x0-x,y0-y,c); }
		}
	}
	void fillCircleHelper(int16_t x0,int16_t y0,int16_t r,uint8_t corner,int16_t delta,uint16_t c){
		int16_t f=1-r, ddF_x=1, ddF_y=-2*r, x=0, y=r;
		while(x<y){
			if(f>=0){ y--; ddF_y+=2; f+=ddF_y; }
			x++; ddF_x+=2; f+=ddF_x;
			if(corner&0x1){ drawFastVLine(x0+x,y0-y,2*y+1+delta,c); drawFastVLine(x0+y,y0-x,2*x+1+delta,c); }
			if(corner&0x2){ drawFastVLine(x0-x,y0-y,2*y+1+delta,c); drawFastVLine(x0-y,y0-x,2*x+1+delta,c); }
		}
	}
	void drawRoundRect(int16_t x,int16_t y,int16_t w,int16_t h,int16_t r,uint16_t c){
		drawFastHLine(x+r, y,     w-2*r, c);
		drawFastHLine(x+r, y+h-1, w-2*r, c);
		drawFastVLine(x,     y+r, h-2*r, c);
		drawFastVLine(x+w-1, y+r, h-2*r, c);
		drawCircleHelper(x+r,     y+r,     r, 1, c);
		drawCircleHelper(x+w-r-1, y+r,     r, 2, c);
		drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, c);
		drawCircleHelper(x+r,     y+h-r-1, r, 8, c);
	}
	void fillRoundRect(int16_t x,int16_t y,int16_t w,int16_t h,int16_t r,uint16_t c){
		fillRect(x+r, y, w-2*r, h, c);
		fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, c);
		fillCircleHelper(x+r,     y+r, r, 2, h-2*r-1, c);
	}
	void fillTriangle(int16_t x0,int16_t y0,int16_t x1,int16_t y1,int16_t x2,int16_t y2,uint16_t c){
		int ymin = y0<y1?(y0<y2?y0:y2):(y1<y2?y1:y2);
		int ymax = y0>y1?(y0>y2?y0:y2):(y1>y2?y1:y2);
		for(int y=ymin;y<=ymax;y++){
			int lo=1<<30, hi=-(1<<30);
			int xs[3]={x0,x1,x2}, ys[3]={y0,y1,y2};
			for(int e=0;e<3;e++){
				int ax=xs[e],ay=ys[e],bx=xs[(e+1)%3],by=ys[(e+1)%3];
				if(ay==by) continue;
				if((y>=ay&&y<=by)||(y>=by&&y<=ay)){
					int x = ax + (bx-ax)*(y-ay)/(by-ay);
					if(x<lo)lo=x; if(x>hi)hi=x;
				}
			}
			if(hi>=lo) for(int x=lo;x<=hi;x++) px(x,y,c);
		}
	}
	void drawBitmap(int16_t x,int16_t y,const uint8_t* bmp,int16_t w,int16_t h,uint16_t c){
		if(!bmp){ nullbmp++; return; }
		for(int16_t j=0;j<h;j++) for(int16_t i=0;i<w;i++){
			if(bmp[j*((w+7)/8)+i/8] & (128>>(i&7))) px(x+i,y+j,c);
		}
	}
	void drawCircle(int16_t x0,int16_t y0,int16_t r,uint16_t c){
		int16_t f=1-r, ddF_x=1, ddF_y=-2*r, x=0, y=r;
		px(x0,y0+r,c); px(x0,y0-r,c); px(x0+r,y0,c); px(x0-r,y0,c);
		while(x<y){
			if(f>=0){ y--; ddF_y+=2; f+=ddF_y; }
			x++; ddF_x+=2; f+=ddF_x;
			px(x0+x,y0+y,c); px(x0-x,y0+y,c); px(x0+x,y0-y,c); px(x0-x,y0-y,c);
			px(x0+y,y0+x,c); px(x0-y,y0+x,c); px(x0+y,y0-x,c); px(x0-y,y0-x,c);
		}
	}
	void fillCircle(int16_t x,int16_t y,int16_t r,uint16_t c){ for(int16_t j=-r;j<=r;j++)for(int16_t i=-r;i<=r;i++) if(i*i+j*j<=r*r) px(x+i,y+j,c); }
	void drawTriangle(int16_t x0,int16_t y0,int16_t x1,int16_t y1,int16_t x2,int16_t y2,uint16_t c){
		drawLine(x0,y0,x1,y1,c); drawLine(x1,y1,x2,y2,c); drawLine(x2,y2,x0,y0,c);
	}
	void setCursor(int16_t x,int16_t y){ cx=x; cy=y; }
	void setTextColor(uint16_t c){ tc=c; }
	void setTextColor(uint16_t c,uint16_t b){ tc=c; (void)b; }
	void setTextSize(uint8_t s){ ts=s?s:1; }
	void setTextWrap(bool){}
	/* print() PHẢI dịch con trỏ như Adafruit_GFX thật, vì code gọi print()
	 * nhiều lần liên tiếp để ghép chuỗi với số. */
	void glyph(unsigned char c){
		for(int col=0;col<5;col++){
			unsigned char bits = GLCD_FONT[c*5+col];
			for(int row=0;row<8;row++)
				if(bits&(1<<row))
					for(int sy=0;sy<ts;sy++) for(int sx=0;sx<ts;sx++)
						px(cx+col*ts+sx, cy+row*ts+sy, tc);
		}
		cx += 6*ts;
	}
	size_t print(const char* s){ if(!s){nullstr++;return 0;} size_t n=0;
		for(;*s;s++,n++) glyph((unsigned char)*s);
		return n; }
	size_t print(int v){ char b[24]; snprintf(b,sizeof b,"%d",v); return print(b); }
	size_t print(unsigned v){ char b[24]; snprintf(b,sizeof b,"%u",v); return print(b); }
	size_t print(unsigned long v){ char b[24]; snprintf(b,sizeof b,"%lu",v); return print(b); }
	size_t print(long v){ char b[24]; snprintf(b,sizeof b,"%ld",v); return print(b); }
	size_t print(char ch){ char b[2]={ch,0}; return print(b); }
	int16_t height(){return HEIGHT;} int16_t width(){return WIDTH;}
	/* instrumentation */
	int oob=0, nullbmp=0, nullstr=0;
	uint8_t fb[128*64];
protected:
	const int16_t WIDTH,HEIGHT;
	int16_t cx=0,cy=0; uint16_t tc=1; uint8_t ts=1;
};
#endif
