/*
 * Breakout game engine
 *
 * Copyright (c) 2013 Sladeware LLC
 */

#include "bb/os.h"
#include "bb/os/drivers/leds/led_matrix_driver.h"
#include BBOS_PROCESSOR_FILE(pins.h)

#define FALSE 0
#define TRUE !(FALSE)

#define NUM_COLS 8
#define NUM_ROWS 8

#define DEFAULT_FRAME_UPDATE_SPEED 5
#define DEFAULT_PADDLE_POS 0x18

#define BUTTONS_MASK 0xFF /* mask for all necessary game buttons */
#define PAUSE_BUTTON_PIN 7
#define LEFT_BUTTON_PIN 4
#define RIGHT_BUTTON_PIN 3

#define MIN_BALL_DELAY 300 /* ms */
#define MAX_BALL_DELAY 600
#define BALL_DELAY MIN_BALL_DELAY
#define BALL_DELAY_STEP 50 /* ms */

#define IS_LEFT_WALL_TOUCHED(i) frame[i] < 0xC0
#define IS_RIGHT_WALL_TOUCHED(i) frame[i] > 0x3

HUBDATA int row_pins[NUM_ROWS] = { 8,  9, 10, 11, 12, 13, 14, 15};
HUBDATA int col_pins[NUM_COLS] = {20, 21, 22, 23, 24, 25, 26, 27};
HUBDATA unsigned char default_frame[NUM_ROWS] = {
  0xFF, 0xFF, 0xFF, /* breaks */
  0, 0, 0, /* space */
  1 << 3, /* ball */
  DEFAULT_PADDLE_POS /* paddle */
};

HUBDATA unsigned char frame[NUM_ROWS];
HUBDATA char is_initialized = 0;
HUBDATA struct led_matrix_driver_cmd cmd;
HUBDATA unsigned char ball_x;
HUBDATA char ball_x_delta;
HUBDATA unsigned char ball_y;
HUBDATA char ball_y_direction;
HUBDATA uint16_t buttons_mask = 0;
HUBDATA char wait_frame = FALSE;
HUBDATA char wait_buttons = FALSE;
HUBDATA char pause_game = TRUE;
unsigned int start_time;

unsigned int
time_ms()
{
  return (CNT / (CLKFREQ / 1000));
}

void
reset()
{
  struct bbos_message* msg;
  unsigned char i;
  pause_game = TRUE;
  /* Drop ball position */
  ball_x = 6;
  ball_x_delta = -1;
  ball_y = 1 << 3;
  ball_y_direction = 1;
  /* Empty frame */
  for (i = 0; i < 8; i++) {
    frame[i] = 0;
  }
  i = 0;
  /* Occupay this core, the game will be reset! */
  wait_frame = 0;
  start_time = time_ms();
  while (i < 8) {
    if ((msg = bbos_receive_message()) != NULL) {
      switch (msg->label) {
      case DRAW_FRAME_STATUS:
        wait_frame = 0;
        break;
      }
      bbos_delete_message(msg);
    }
    if (wait_frame) {
      continue;
    }
    if ((time_ms() - start_time) < 200) {
      if ((msg = bbos_request_message(LED_MATRIX_DRIVER)) != NULL) {
        msg->label = DRAW_FRAME;
        *((struct led_matrix_driver_cmd*)msg->payload) = cmd;
        bbos_send_message(msg);
        wait_frame = 1;
      }
    }
    else {
      frame[i] = default_frame[i];
      i++;
      start_time = time_ms();
    }
  }
}

/* Flush the screen. */
void
game_over()
{
  struct bbos_message* msg;
  unsigned char i;
  for (i = 0; i < 8; i++) {
    frame[i] = 0xFF;
  }
  start_time = time_ms();
  wait_frame = 0;
  while ((time_ms() - start_time) < 1000) {
    if ((msg = bbos_receive_message()) != NULL) {
      wait_frame = 0;
      bbos_delete_message(msg);
    }
    if (!wait_frame && (msg = bbos_request_message(LED_MATRIX_DRIVER)) != NULL) {
      msg->label = DRAW_FRAME;
      *((struct led_matrix_driver_cmd*)msg->payload) = cmd;
      bbos_send_message(msg);
      wait_frame = 1;
    }
  }
}

void
move_ball()
{
  /* Update ball position  */
  frame[ball_x] ^= ball_y; /* remove the ball from the previous position */
  ball_x += ball_x_delta; /* move ball position */
  if (ball_y_direction == 1) {
    ball_y <<= 1;
  }
  else {
    ball_y >>= 1;
  }
  if (ball_x == 0) {
    ball_x_delta *= -1;
  }
  else if (ball_x == 6) {
    if (!(frame[7] & ball_y)) {
      /* Game over! */
      game_over();
      reset();
      return;
    }
    ball_x_delta *= -1;
    ball_y_direction *= -1;
  }
  /* Does the ball touch the breaks? */
  else if ((ball_x < 3) && (frame[ball_x] & ball_y)) {
    frame[ball_x] ^= ball_y; /* remove the break */
    ball_x_delta *= -1;
    ball_y_direction *= -1;
  }
  else if (ball_y == 0x80 || ball_y == 0x01) {
    ball_y_direction *= -1;
  }
  frame[ball_x] |= ball_y;
}

void
engine_runner()
{
  struct bbos_message* msg;
  while ((msg = bbos_receive_message()) != NULL) {
    switch (msg->label) {
    case PRESSED_BUTTONS:
      buttons_mask = *((uint16_t*)msg->payload);
      if (buttons_mask & (1 << PAUSE_BUTTON_PIN)) {
        pause_game ^= TRUE;
      }
      wait_buttons = 0;
      break;
    case DRAW_FRAME_STATUS:
      wait_frame = 0;
      break;
    }
    bbos_delete_message(msg);
  }
  if (is_initialized != 1) {
    /* Send open request */
    if ((msg = bbos_request_message(LED_MATRIX_DRIVER)) != NULL) {
      msg->label = BBOS_DRIVER_OPEN;
      cmd.speed = DEFAULT_FRAME_UPDATE_SPEED;
      cmd.num_rows = cmd.num_cols = 8; /* 8x8 matrix */
      cmd.row_pins = row_pins;
      cmd.col_pins = col_pins;
      cmd.frame = frame;
      *((struct led_matrix_driver_cmd*)msg->payload) = cmd;
      bbos_send_message(msg);
      reset();
      is_initialized = 1;
    }
    start_time = time_ms();
    return;
  }
  /* Update the frame */
  if (!wait_frame && ((msg = bbos_request_message(LED_MATRIX_DRIVER)) != NULL)) {
    if (!pause_game) {
      /* Update paddle position */
      if (buttons_mask & (1 << LEFT_BUTTON_PIN) && IS_LEFT_WALL_TOUCHED(7)) {
        frame[7] <<= 1;
      }
      if ((buttons_mask & (1 << RIGHT_BUTTON_PIN)) && IS_RIGHT_WALL_TOUCHED(7)) {
        frame[7] >>= 1;
      }
      /* Update ball position? */
      if ((time_ms() - start_time) > BALL_DELAY) {
        move_ball();
        start_time = time_ms();
      }
    }
    buttons_mask = 0;
    msg->label = DRAW_FRAME;
    *((struct led_matrix_driver_cmd*)msg->payload) = cmd;
    bbos_send_message(msg);
    wait_frame = 1;
  }
  /* Request buttons bits */
  if (!wait_buttons && (msg = bbos_request_message(BUTTON_DRIVER)) != NULL) {
    msg->label = ARE_BUTTONS_PRESSED;
    *((uint16_t*)msg->payload) = BUTTONS_MASK;
    bbos_send_message(msg);
    wait_buttons = 1;
  }
}
