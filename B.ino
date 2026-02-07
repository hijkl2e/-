int state[5];
int lastState[5];
unsigned long lastDebounceTime[5];
unsigned long debounceDelay = 50;

int mode = -1;
int num = -1;

/**
 * @brief 초기 핀 설정 및 스위치 상태 초기화 함수
 */
void setup() {
  for (int i = 2; i < 5; ++i) { // 스위치
    pinMode(i, INPUT);
  }
  for (int i = 5; i < 8; ++i) { // RGB LED
    pinMode(i, OUTPUT);
  }
  for (int i = 8; i < 12; ++i) { // BCD to 7-Segment 디코더
    pinMode(i, OUTPUT);
  }
  for (int i = 12; i < 14; ++i) { // 7-Segment LED
    pinMode(i, OUTPUT);
  }
}

/**
 * @brief 스위치 입력을 디바운싱 처리하여 상태 변화를 감지하는 함수
 *
 * 각 스위치의 입력을 확인하여 눌림/떼짐 상태 변화가
 * 안정적으로 발생했을 경우 해당 스위치의 핀 번호를 반환한다.
 *
 * @return 상태 변화가 감지된 스위치의 핀 번호,
 *         변화가 없으면 -1 반환
 */
int SWITCH() {
  for (int i = 2; i < 5; ++i) {
    int reading = digitalRead(i);
    if (lastState[i] != reading) {
      lastState[i] = reading;
      lastDebounceTime[i] = millis();
    } else if (millis() - lastDebounceTime[i] > debounceDelay) {
      if (state[i] != reading) {
        state[i] = reading;
        return i;
      }
    }
  }
  return -1;
}

/**
 * @brief 스위치 상태 초기화 함수
 */
void CLEAR_SWITCH() {
  for (int i = 2; i < 5; ++i) {
    state[i] = HIGH;
    lastState[i] = HIGH;
    lastDebounceTime[i] = 0;
  }
}

/**
 * @brief LED 출력 제어 함수
 *
 * 지정된 번호에 해당하는 LED만 점등하고 나머지는 소등한다.
 *
 * @param c LED 인덱스 (1: R, 2: G, 3: B)
 */
void LED(int c) {
  for (int i = 5; i < 8; ++i) {
    digitalWrite(i, c == (i - 4));
  }
}

/**
 * @brief 1자리 숫자를 FND에 표시하는 함수
 *
 * 두 FND에 동일한 1자리 숫자를 표시한다.
 *
 * @param n 표시할 숫자 (-1이면 소등)
 */
void DIGIT1(int n) {
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
  if (n == -1) {
    return;
  }
  num = 11 * n;
  for (int i = 8; i < 12; ++i) {
    digitalWrite(i, (n >> (i - 8)) & 1);
  }
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
}

/**
 * @brief 2자리 숫자를 FND에 표시하는 함수
 *
 * 2자리 숫자를 멀티플렉싱 방식으로 표시한다.
 *
 * @param n 표시할 숫자
 */
void DIGIT2(int n) {
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
  num = n;
  for (int k = 12; k < 14; ++k) {
    int m = k == 12 ? (n / 10) : (n % 10);
    for (int i = 8; i < 12; ++i) {
      digitalWrite(i, (m >> (i - 8)) & 1);
    }
    digitalWrite(k, HIGH);
    delay(10);
    digitalWrite(k, LOW);
  }
}

/**
 * [초기화 동작]
 * - 전원을 최초로 인가하거나 SW3을 누르는 경우 다음의 [초기화 동작]을 수행하도록
 *   합니다.
 *  1) FND에 숫자 88을 1초 점등, 1초 소등을 2회 반복한 다음 숫자 50를 표시합니다.
 *  2) RGB LED 모듈은 순서대로 G(녹색)가 1초 점등, B(청색)가 1초 점등을 2회 반복한
 *     다음 R(적색)이 점등합니다.
 *  3) 수행이 완료한 상태를 [초기화 상태]로 하여 숫자 50과 아두이노 RGB LED 모듈의
 *     R(적색) 점등 상태를 유지합니다.
 *  4) [초기화 동작]을 수행하는 도중에는 어떠한 스위치 입력도 무시합니다.
 */
int INIT() {
  for (int i = 1; i < 5; ++i) {
    DIGIT1(i % 2 ? 8 : -1);
    LED(i % 2 ? 2 : 3);
    delay(1000);
  }
  num = 50;
  return 0;
}

/**
 * @brief FND는 현재 값을 유지하고 RGB 모듈은 R(적색) 점등 상태를 유지
 */
int NONE() {
  LED(1);
  while (true) {
    DIGIT2(num);
    int s = SWITCH();
    if (s == 4 && !state[s]) {
      return -1;
    } else if (!state[2] && millis() - lastDebounceTime[2] >= 1000) {
      return 1;
    } else if (!state[3] && millis() - lastDebounceTime[3] >= 1000) {
      return 2;
    }
  }
}

/**
 * [동작 1]
 * - SW1을 1초 이상 누르고 있는 경우 스위치를 누르고 있는 동안 다음의 [동작 1]을
 *   수행하도록 합니다.
 *  1) FND의 표기 숫자는 0.5초에 숫자를 1씩 감소하고 RGB LED 모듈은 0.5초간 B(청색)를
 *     점등하고 0.5초간 소등하는 동작을 반복합니다.
 *  2) [동작 1]을 수행하는 도중에는 SW2과 SW3 스위치 입력을 무시합니다.
 *  3) [동작 1]을 수행하는 중 SW1을 떼면 [동작 1]을 종료하며 FND는 현재 값을 유지하고
 *     RGB 모듈은 R(적색) 점등 상태를 유지합니다.
 *  4) FND의 표기 숫자가 00에 도달한 뒤로도 1초 이상 SW1를 누르고 있으면 [동작 1]
 *     수행을 종료하고 [동작 3]을 수행합니다.
 */
int MODE1() {
  if (num == 0) {
    return 3;
  }
  int c = 0;
  bool f = false;
  unsigned long time = millis() - 500;
  while (true) {
    if (millis() - time >= 500) {
      if (num > 0) {
        --num;
      } else if (!f) {
        f = true;
      } else {
        return 3;
      }
      c ^= 3;
      time += 500;
    }
    DIGIT2(num);
    LED(c);
    int s = SWITCH();
    if (s == 2 && state[s]) {
      return 0;
    }
  }
}

/**
 * [동작 2]
 * - SW2를 1초 이상 누르고 있는 경우 스위치를 누르고 있는 동안 다음의 [동작 2]을
 *   수행하도록 합니다.
 *  1) FND의 표기 숫자는 0.5초에 숫자를 1씩 증가하고 RGB LED 모듈은 0.5초간 G(녹색)를
 *     점등하고 0.5초간 소등하는 동작을 반복합니다.
 *  2) [동작 2]를 수행하는 도중에는 SW1과 SW3 스위치 입력을 무시합니다.
 *  3) [동작 2]를 수행하는 중 SW2을 떼면 [동작 2]를 종료하며 FND는 현재 값을 유지하고
 *     RGB 모듈은 R(적색) 점등 상태를 유지합니다.
 *  4) FND의 표기 숫자가 99에 도달한 뒤로도 1초 이상 SW2를 누르고 있으면 [동작 2]
 *     수행을 종료하고 [동작 3]을 수행합니다.
 */
int MODE2() {
  if (num == 99) {
    return 3;
  }
  int c = 0;
  bool f = false;
  unsigned long time = millis() - 500;
  while (true) {
    if (millis() - time >= 500) {
      if (num < 99) {
        ++num;
      } else if (!f) {
        f = true;
      } else {
        return 3;
      }
      c ^= 2;
      time += 500;
    }
    DIGIT2(num);
    LED(c);
    int s = SWITCH();
    if (s == 3 && state[s]) {
      return 0;
    }
  }
}

/**
 * [동작 3]
 * - FND의 표기 숫자가 00일 때 SW1을 1초 이상 누르고 있거나, FND의 표기 숫자가 99일
 *   때 SW2를 1초이상 누르고 있으면 [동작 3]을 수행합니다.
 *  1) FND는 00을 1초, 99를 1초 표기하는 것을 2회 반복하고 RGB 모듈은 R(적색)이 1초
 *     점등, G(녹색)가 1초 점등, B(청색)가 1초 점등, 1초 소등합니다.
 *  2) 위 동작을 수행 완료한 후 FND 표시 숫자 50, RGB 모듈 R(적색) 점등의 [초기화
 *     상태]로 돌아갑니다.
 *  3) [동작 3]을 수행하는 도중에는 어떠한 스위치 입력도 무시합니다.
 */
int MODE3() {
  for (int i = 1; i < 5; ++i) {
    DIGIT1(i % 2 ? 0 : 9);
    LED(i);
    delay(1000);
  }
  num = 50;
  return 0;
}

/**
 * @brief 메인 루프 함수
 *
 * 현재 설정된 mode 값에 따라
 * INIT, NONE, MODE1, MODE2, MODE3 중 하나를 실행한다.
 */
void loop() {
  CLEAR_SWITCH();
  if (mode == -1) {
    mode = INIT();
  } else if (mode == 0) {
    mode = NONE();
  } else if (mode == 1) {
    mode = MODE1();
  } else if (mode == 2) {
    mode = MODE2();
  } else if (mode == 3) {
    mode = MODE3();
  }
}
