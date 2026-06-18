#ifndef __SCOREBOARD_H
#define __SCOREBOARD_H

/* Implemented in scoreboard.S (RISC-V assembly).
 *
 *  scoreboard_update : insert (level, score) into the persistent top-3,
 *                      keeping it sorted: score desc, then level desc.
 *                      Called after EVERY death.
 *  scoreboard_show   : render the top-3 and block until SW1 is pressed.
 *
 * All record-keeping / comparison / sorting is done with assembly
 * instructions; only leaf helpers (LCD_*, Get_Button, delay_1ms) are C. */
void scoreboard_update(int level, int score);
void scoreboard_show(void);

#endif /* __SCOREBOARD_H */
