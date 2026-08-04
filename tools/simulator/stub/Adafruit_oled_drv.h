#ifndef __ADAFRUIT_OLED_DRV_H__
#define __ADAFRUIT_OLED_DRV_H__
#include "Adafruit_GFX.h"
class Adafruit_oled_drv : public Adafruit_GFX {
public:
	Adafruit_oled_drv():Adafruit_GFX(128,64){}
	bool initialize(){ inited=true; return true; }
	void update(){ updates++; }
	void updateRow(int){ updates++; }
	void updateRow(int,int){ updates++; }
	void clear(bool u=false){ memset(fb,0,sizeof(fb)); if(u)update(); }
	void display_on(){ on=true; }
	void display_off(){ on=false; }
	bool inited=false, on=false; long updates=0;
};
#endif
