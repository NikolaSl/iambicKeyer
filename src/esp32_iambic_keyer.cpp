#include <Arduino.h>

#define WPM_SPEED 15
#define WPM_MIN_SPEED 5
#define WPM_MAX_SPEED 30
#define TONE_FREQ 550

#define DIT_PADDLE_PIN 12
#define DAH_PADDLE_PIN 13
#define TONE_PIN 27
#define SWITCH_PIN 26
#define SET_SPEED_PIN 14

enum State {PAUSE=0, DIT=1, DAH=2, DITDAH=3, DAHDIT=4};
enum Played {NONE=0, DOT=1, DASH=2};

Played last;
State state;
uint32_t wpm;
uint32_t ditMilliseconds;
uint32_t dahMilliseconds;
uint32_t gapMilliseconds;

void setWordsPerMinute(uint32_t wordsPerMinute) {
  wpm = wordsPerMinute;
  ditMilliseconds = 1200/wpm;
  dahMilliseconds = 3 * ditMilliseconds;
  gapMilliseconds = ditMilliseconds;
}

void IRAM_ATTR updatePaddles() {
  boolean dit = digitalRead(DIT_PADDLE_PIN);
  boolean dah = digitalRead(DAH_PADDLE_PIN);
  boolean setSpeed = digitalRead(SET_SPEED_PIN);
  if (setSpeed == LOW) {
    if (dit == LOW && wpm > WPM_MIN_SPEED) wpm--;
    if (dah == LOW && wpm < WPM_MAX_SPEED) wpm++;
    setWordsPerMinute(wpm);
  } else {
    switch (state) {
      case PAUSE:
        if (dit == LOW) {
          if (dah == LOW) {
            state = DITDAH;
          } else {
            state = DIT;
          }
        } else if (dah == LOW) {
          state = DAH;
        }
        break;
      case DIT:
        if (dah == LOW) {
          if (dit == LOW) {
            state = DITDAH;
          } else {
            state = DAH;
          }
        } else if (dit == HIGH) {
          state = PAUSE;
        }
        break;
      case DAH:
        if (dit == LOW) {
          if (dah == LOW) {
            state = DAHDIT;
          } else {
            state = DIT;
          }
        } else if (dah == HIGH) {
          state = PAUSE;
        }
        break;
      case DITDAH:
      case DAHDIT:
        if (dit == HIGH) {
          if (dah == HIGH) {
            state = PAUSE;
          } else {
            state = DAH;
          }
        } else if (dah == HIGH) {
          state = DIT;
        }
        break;
    }
  }
}

void setup() {
  last = NONE;
  state = PAUSE;
  setWordsPerMinute(WPM_SPEED);
  ledcSetup(0, TONE_FREQ, 10);
  ledcAttachPin(TONE_PIN, 0);
  pinMode(DIT_PADDLE_PIN, INPUT_PULLUP);
  pinMode(DAH_PADDLE_PIN, INPUT_PULLUP);
  pinMode(SET_SPEED_PIN, INPUT_PULLUP);
  pinMode(SWITCH_PIN, OUTPUT);
  attachInterrupt(DIT_PADDLE_PIN, updatePaddles, CHANGE);
  attachInterrupt(DAH_PADDLE_PIN, updatePaddles, CHANGE);
}

void morsePlay(Played p) {
  ledcWriteTone(0, TONE_FREQ);
  digitalWrite(SWITCH_PIN, HIGH);
  if (p == DOT) {
      delay(ditMilliseconds);
  } else {
      delay(dahMilliseconds);
  }
  digitalWrite(SWITCH_PIN, LOW);
  ledcWriteTone(0, 0);
  delay(gapMilliseconds);
  last = p;
}

void loop() {
  switch (state) {
    case DIT:
      morsePlay(DOT);
      break;
    case DAH:
      morsePlay(DASH);
      break;
    case DITDAH:
    case DAHDIT:
      if (last == NONE) {
        if (state == DITDAH) {
          morsePlay(DOT);
        } else {
          morsePlay(DASH);
        }
      } else if (last == DOT) {
        morsePlay(DASH);
      } else {
        morsePlay(DOT);
      }
      break;
    case PAUSE:
      last = NONE;
  }
}
