/***********************************************************************
  Sample program

   Generated at 2014-11-25 09:44:07

 ***********************************************************************/

/* {{UC3_INCLUDE */
#include "kernel.h"
#include "kernel_id.h"
#include "hw_dep.h"
#include "net_hdr.h"
#include "net_id.h"
#include "http_server.h"
/* }}UC3_INCLUDE */
#include "modbus.h"


extern void cortex_m_init_peripheral(void);
extern void init_peripheral(void);

/* {{UC3_CODE */


/******************************************************************
    Default interrupt handler
     Reached when the interrupt is accepted even though it is not the generation of an
     interrupt service routine also define the interrupt handler.
 ******************************************************************/

void int_abort(void)
{
    for(;;);
}

/*******************************
        cyc_func1
 *******************************/
/* {{UC3_CYC(cyc_func1) */
void cyc_func1(VP_INT exinf)
{
}
/* }}UC3_CYC */

/*******************************
        inti_tsk
 *******************************/
extern ER net_setup(void);
/* {{UC3_TASK(inti_tsk) */
void inti_tsk(VP_INT exinf)
{
    ER ercd;


    /* Initialize uNet3 */
    ercd = net_setup();
    if (ercd != E_OK) {
        return;
    }

	mbstack_poll(  );

    for (;;)
	{
		mbstack_poll(  );
		dly_tsk(1);
    }
}
/* }}UC3_TASK */

/*******************************
        update_tsk
 *******************************/
/* {{UC3_TASK(update_tsk) */
void update_tsk(VP_INT exinf)
{
}
/* }}UC3_TASK */

/*******************************
        serial_snd_rcv_tsk
 *******************************/
/* {{UC3_TASK(serial_snd_rcv_tsk) */
void serial_snd_rcv_tsk(VP_INT exinf)
{
}
/* }}UC3_TASK */

/*******************************
        lan_snd_rcv_tsk
 *******************************/
/* {{UC3_TASK(lan_snd_rcv_tsk) */
void lan_snd_rcv_tsk(VP_INT exinf)
{
}
/* }}UC3_TASK */

/*******************************
        cnf_mgt_tsk
 *******************************/
/* {{UC3_TASK(cnf_mgt_tsk) */
void cnf_mgt_tsk(VP_INT exinf)
{
}
/* }}UC3_TASK */

/*******************************
        task_mbmaster
 *******************************/
/* {{UC3_TASK(task_mbmaster) */
void task_mbmaster(VP_INT exinf)
{
    task_mbmaster_impl( );
}
/* }}UC3_TASK */

/* }}UC3_CODE */

void init_register(void)
{
	REG_GPIO.PFR0 = 0x0000FC3F;
	REG_GPIO.PFR1 = 0x00000F80;
	REG_GPIO.PFR2 = 0x0000FC37;
	REG_GPIO.PFR3 = 0x00000D00;
	REG_GPIO.PFR4 = 0x0000FCC1;
	REG_GPIO.PFR5 = 0x0000C59F;
	REG_GPIO.PFR6 = 0x0000FFFB;
	REG_GPIO.PFR7 = 0x00000FFF;
	REG_GPIO.PFR8 = 0x0000FFF3;
	REG_GPIO.PFR9 = 0x0000FFC1;
	REG_GPIO.PFRA = 0x0000FFC0;
	REG_GPIO.PFRB = 0x0000FF00;
	REG_GPIO.PFRC = 0x000001F8;
	REG_GPIO.PFRD = 0x0000FFFE;
    REG_GPIO.PFRE = 0x0000FFFF;
	REG_GPIO.PFRF = 0x0000FF80;

	REG_GPIO.DDR3 = 0x0000FF00;
	REG_GPIO.PDOR3 = 0x00000000;

	REG_GPIO.ADE = 0x00000000;

	REG_GPIO.EPFR00 = 0x00032000;
	//REG_GPIO.EPFR01;
	//REG_GPIO.EPFR02;
	//REG_GPIO.EPFR03;
	//REG_GPIO.EPFR04;
	//REG_GPIO.EPFR05;
	REG_GPIO.EPFR06 = 0x82000002;
	REG_GPIO.EPFR07 = 0x03FF0000;
	REG_GPIO.EPFR08 = 0x000000A0;
	//REG_GPIO.EPFR09;
	REG_GPIO.EPFR10 = 0x780001C;
	//REG_GPIO.EPFR11;
	//REG_GPIO.EPFR12;
	//REG_GPIO.EPFR13;
	REG_GPIO.EPFR14 = 0x21540000;
	//REG_GPIO.EPFR15;
	REG_GPIO.SPSR = 0x07;
}


/*******************************
        Main entry
 *******************************/

int main(void)
{
    ER ret = E_OK;

    /* Initialize hardware */
    cortex_m_init_peripheral();
    init_peripheral();
	init_register();

    /* Generate system and start */
    ret = start_uC3();

    return ret;
}

/* end */
