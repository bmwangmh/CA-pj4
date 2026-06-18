# =====================================================================
#  scoreboard.S  --  persistent top-3 scoreboard (RISC-V assembly)
#
#  Required by Project 4 spec 1.1: the score recording, sorting and
#  ranking display must be done in assembly.  We only call C *leaf*
#  helpers for I/O (LCD_Clear / LCD_ShowString / LCD_ShowNum /
#  Get_Button / delay_1ms) -- never any C routine that does the
#  comparison / sorting / data processing.
#
#  Storage lives in .data so it survives between games (soft state) but
#  is wiped by a hard reset, exactly as required.
#
#  Sort order (descending):
#     primary   : score  (higher first)
#     secondary : level  (higher first, i.e. 1-3 > 1-2 > 1-1)
#  Empty slots use score = -1 so a real entry (score >= 0) always beats
#  them.
# =====================================================================

    .section .data
    .align 2
sb_score:                       # current top-3 scores (descending)
    .word -1, -1, -1
sb_level:                       # matching level (1,2,3)
    .word  0,  0,  0

    .section .rodata
    .align 2
str_title:  .asciz "SCOREBOARD"
str_lv:     .asciz "1-"
str_colon:  .asciz ":"
str_empty:  .asciz "---"
str_back:   .asciz "SW1:back"

# Colour / button constants (mirror lcd.h / utils.h)
    .equ C_BLACK,  0x0000
    .equ C_WHITE,  0xFFFF
    .equ C_YELLOW, 0xFFE0
    .equ BTN_SW1,  5

# =====================================================================
#  void scoreboard_update(int level /*a0*/, int score /*a1*/)
#
#  Insertion into a 3-element array kept sorted descending.
# =====================================================================
    .section .text
    .globl scoreboard_update
    .type  scoreboard_update, @function
scoreboard_update:
    la   t0, sb_score           # t0 = &sb_score[0]
    la   t1, sb_level           # t1 = &sb_level[0]
    li   t2, 0                  # t2 = i = 0

.Lfind:
    li   t3, 3
    beq  t2, t3, .Ldone         # i == 3 -> not in top-3, drop it

    slli t4, t2, 2              # t4 = i*4
    add  t5, t0, t4
    lw   t6, 0(t5)              # t6 = sb_score[i]

    blt  t6, a1, .Linsert       # new score  >  slot score  -> insert here
    blt  a1, t6, .Lnext         # new score  <  slot score  -> keep scanning
    # equal score: compare level
    add  t5, t1, t4
    lw   t6, 0(t5)              # t6 = sb_level[i]
    blt  t6, a0, .Linsert       # new level  >  slot level   -> insert here
.Lnext:
    addi t2, t2, 1
    j    .Lfind

.Linsert:
    # shift slots [i .. 1] down by one: for j = 2; j > i; j--
    li   t3, 2                  # t3 = j
.Lshift:
    bge  t2, t3, .Lplace        # while j > i
    slli t4, t3, 2              # t4 = j*4
    addi t5, t4, -4            # t5 = (j-1)*4
    # sb_score[j] = sb_score[j-1]
    add  a4, t0, t5
    lw   a5, 0(a4)
    add  a4, t0, t4
    sw   a5, 0(a4)
    # sb_level[j] = sb_level[j-1]
    add  a4, t1, t5
    lw   a5, 0(a4)
    add  a4, t1, t4
    sw   a5, 0(a4)
    addi t3, t3, -1
    j    .Lshift

.Lplace:
    slli t4, t2, 2             # t4 = i*4
    add  a4, t0, t4
    sw   a1, 0(a4)             # sb_score[i] = new score
    add  a4, t1, t4
    sw   a0, 0(a4)             # sb_level[i] = new level
.Ldone:
    ret

# =====================================================================
#  void scoreboard_show(void)
#
#  Draw the board, then poll until SW1 is pressed.
#  Saved registers used to survive the C calls:
#     s0 = i (row 0..2)
#     s1 = y pixel for current row
#     s2 = sb_score[i] for current row
#     s3 = sb_level[i] for current row
# =====================================================================
    .globl scoreboard_show
    .type  scoreboard_show, @function
scoreboard_show:
    addi sp, sp, -32
    sw   ra, 28(sp)
    sw   s0, 24(sp)
    sw   s1, 20(sp)
    sw   s2, 16(sp)
    sw   s3, 12(sp)

    # clear screen
    li   a0, C_BLACK
    call LCD_Clear

    # title at (28, 0)
    li   a0, 28
    li   a1, 0
    la   a2, str_title
    li   a3, C_YELLOW
    call LCD_ShowString

    li   s0, 0                  # i = 0
.Lrow:
    li   t0, 3
    bge  s0, t0, .Lwait

    # y = 16 + i*16
    slli s1, s0, 4
    addi s1, s1, 16

    # load this row's score/level
    la   t0, sb_score
    slli t1, s0, 2
    add  t0, t0, t1
    lw   s2, 0(t0)             # s2 = score
    la   t0, sb_level
    add  t0, t0, t1
    lw   s3, 0(t0)             # s3 = level

    # "1-" at x=0
    li   a0, 0
    mv   a1, s1
    la   a2, str_lv
    li   a3, C_WHITE
    call LCD_ShowString

    # skip number drawing if slot empty (score < 0)
    bltz s2, .Lempty

    # level digit at x=16  : LCD_ShowNum(16, y, level, 1, WHITE)
    li   a0, 16
    mv   a1, s1
    mv   a2, s3
    li   a3, 1
    li   a4, C_WHITE
    call LCD_ShowNum

    # ":" at x=24
    li   a0, 24
    mv   a1, s1
    la   a2, str_colon
    li   a3, C_WHITE
    call LCD_ShowString

    # score at x=32 : LCD_ShowNum(32, y, score, 4, WHITE)
    li   a0, 32
    mv   a1, s1
    mv   a2, s2
    li   a3, 4
    li   a4, C_WHITE
    call LCD_ShowNum
    j    .Lrowend

.Lempty:
    # "---" at x=16
    li   a0, 16
    mv   a1, s1
    la   a2, str_empty
    li   a3, C_WHITE
    call LCD_ShowString

.Lrowend:
    addi s0, s0, 1
    j    .Lrow

.Lwait:
    # footer hint at (0, 64)
    li   a0, 0
    li   a1, 64
    la   a2, str_back
    li   a3, C_YELLOW
    call LCD_ShowString

.Lpoll:
    li   a0, BTN_SW1
    call Get_Button
    bnez a0, .Lleave
    li   a0, 10
    call delay_1ms
    j    .Lpoll

.Lleave:
    lw   ra, 28(sp)
    lw   s0, 24(sp)
    lw   s1, 20(sp)
    lw   s2, 16(sp)
    lw   s3, 12(sp)
    addi sp, sp, 32
    ret
