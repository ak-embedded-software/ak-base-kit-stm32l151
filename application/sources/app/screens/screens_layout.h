/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @brief:  Hằng số bố cục dùng chung cho mọi màn hình.
 *
 *  Màn OLED 128x64. Không màn nào vẽ khung viền ngoài. Thay vào đó mọi thứ
 *  chừa lề 3 px bốn phía:
 *
 *      x = SCR_PAD_L .. SCR_PAD_R   (3 .. 124, tức 122 px = 20 ký tự)
 *      y = SCR_PAD_T .. SCR_PAD_B   (3 .. 60,  tức 58 dòng)
 *
 *  Chỉ có hai chỗ cố ý tràn viền:
 *    1. Dải hang ở màn TRAVEL. Nó là đường di chuyển, phải chạm hai rìa mới ra
 *       cảm giác hang dài vô tận. Xem dungeon_draw_travel().
 *    2. Màn scr_idle - mấy quả bóng nảy. Nó là screensaver, nảy chạm mép mới
 *       đúng nghĩa.
 *  Còn lại mọi màn đều phải nằm trong lề. Có script kiểm tra tự động dump
 *  framebuffer rồi quét từng pixel, không màn nào được lọt ra ngoài.
 *
 *  MẤY BỐ CỤC CHIA HÀNG HAY DÙNG (vùng dọc chỉ có 58 dòng nên nhớ sẵn):
 *      3 thẻ cao 18, khe 2      -> 18*3 + 2*2 = 58   (màn Menu)
 *      4 hàng cao 13, khe 2     -> 13*4 + 2*3 = 58   (màn Setting)
 *      2 dòng chữ cỡ 2 + 2 dòng chữ cỡ 1               (màn Game Over)
 *
 *  LƯU Ý VỀ FONT: glcdfont là 5x8, KHÔNG phải 5x7. Chữ cỡ 1 đặt ở cursor y
 *  chiếm y..y+7. Phần lớn ký tự chỉ dùng 7 dòng trên, nhưng chữ có nét thả
 *  như 'y' 'g' 'p' 'q' dùng tới dòng thứ 8. Nên:
 *
 *      hàng chữ cỡ 1 cuối cùng phải đặt ở y <= 53
 *      hàng chữ cỡ 2 cuối cùng phải đặt ở y <= 45   (cao 16 dòng)
 *
 *  Mỗi ký tự cỡ 1 chiếm 6 px ngang. Số ký tự tối đa của chuỗi bắt đầu ở x là
 *  SCR_MAX_CHARS(x). Dùng SCR_CENTER_X(n) để canh giữa chuỗi n ký tự.
 ******************************************************************************
**/
#ifndef __SCREENS_LAYOUT_H__
#define __SCREENS_LAYOUT_H__

#define SCR_PAD_L               (3)
#define SCR_PAD_R               (124)
#define SCR_PAD_T               (3)
#define SCR_PAD_B               (60)

#define SCR_USABLE_W            (SCR_PAD_R - SCR_PAD_L + 1)   /* 122 */

/* Chiều cao glyph theo cỡ chữ. */
#define SCR_CHAR_W              (6)
#define SCR_CHAR_H              (8)

/* Hàng chữ cuối cùng có thể đặt, theo cỡ chữ. */
#define SCR_ROW_LAST            (SCR_PAD_B - SCR_CHAR_H + 1)          /* 53 */
#define SCR_ROW_LAST_BIG        (SCR_PAD_B - (SCR_CHAR_H * 2) + 1)    /* 45 */

/* Số ký tự cỡ 1 nhét được nếu bắt đầu ở cột x. */
#define SCR_MAX_CHARS(x)        ((SCR_PAD_R + 1 - (x)) / SCR_CHAR_W)

/* Toạ độ x để canh giữa chuỗi n ký tự trong vùng có lề. */
#define SCR_CENTER_X(n)         (SCR_PAD_L + ((SCR_USABLE_W - ((n) * SCR_CHAR_W)) / 2))
#define SCR_CENTER_X_BIG(n)     (SCR_PAD_L + ((SCR_USABLE_W - ((n) * SCR_CHAR_W * 2)) / 2))

/* Mấy hàng dùng lại ở nhiều màn. */
#define SCR_ROW_TITLE           (3)     /* tiêu đề trên cùng           */
#define SCR_ROW_RULE            (11)    /* dòng kẻ dưới tiêu đề        */
#define SCR_ROW_BODY            (15)    /* dòng nội dung đầu tiên      */
#define SCR_ROW_HINT            (53)    /* dòng gợi ý thao tác dưới đáy*/

#endif //__SCREENS_LAYOUT_H__
