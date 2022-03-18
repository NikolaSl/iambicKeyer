/*
Iambic-A morse keyer and trainer  
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Nikola Slavchev LZ1NKL <slavchev@gmail.com>.
Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <Arduino.h>

#define WPM_SPEED 20
#define WPM_MIN_SPEED 5
#define WPM_MAX_SPEED 30
#define TONE_FREQ 600
#define DAH_IN_DITS 3
#define GAP_IN_DITS 1
#define MIN_CHAR_GAP_IN_DITS 2
#define MIN_WORD_GAP_IN_DITS 7

#define DIT_PADDLE_PIN 12
#define DAH_PADDLE_PIN 13
#define TONE_PIN 27
#define SWITCH_PIN 26
#define SET_SPEED_PIN 14

enum State {PAUSE=0, DIT=1, DAH=2, DITDAH=3, DAHDIT=4};
enum Played {NONE=0, DOT=1, DASH=2};

Played last;
State state;
ulong wpm;
ulong ditMilliseconds;
ulong dahMilliseconds;
ulong gapMilliseconds;
ulong newCharMilliseconds;
ulong newWordMilliseconds;

void decodeMorse(Played played) {
  const char morseTree[] =  " ETIANMSURWDKGOHVF*L*PJBXCYZQ**54*3***2&*+***'1"\
    "6=/**^(*7***8*90*****$******?_****\"**.****@******-********;!*)*****,****:";
  static ulong morseIndex = 0;
  static ulong timeOfLastBeep = 0;
  static bool lastSpace = false;
  char decodedChar = 0x00;
  if (played == NONE ) {
    if (morseIndex && millis() - timeOfLastBeep > newCharMilliseconds) {
      decodedChar = morseTree[morseIndex];
      morseIndex = 0;
    } else if (!lastSpace && millis() - timeOfLastBeep > newWordMilliseconds) {
      decodedChar = morseTree[morseIndex];
      lastSpace = true;
    }
  } else {
    lastSpace = false;
    morseIndex = 2*morseIndex + (played == DOT ? 1 : 2);
    if (morseIndex > sizeof(morseTree)) {
      decodedChar = '*';
      morseIndex = 0;
    }
  }
  if (decodedChar) {
    Serial.print(decodedChar);
  } else if (played != NONE) {
    timeOfLastBeep = millis();
  }
}

void setWordsPerMinute(uint32_t wordsPerMinute) {
  Serial.printf("\nSwitch to %u words per minute.\n", wordsPerMinute);
  wpm = wordsPerMinute;
  ditMilliseconds = 1200/wpm;
  dahMilliseconds = DAH_IN_DITS * ditMilliseconds;
  gapMilliseconds = GAP_IN_DITS * ditMilliseconds;
  newCharMilliseconds = (MIN_CHAR_GAP_IN_DITS - 1) * ditMilliseconds;
  newWordMilliseconds = (MIN_WORD_GAP_IN_DITS - 1) * ditMilliseconds;
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
  Serial.begin(115200);
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
  decodeMorse(last);
}
