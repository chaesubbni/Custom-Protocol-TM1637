# ATmega328P-Real-Time-Clock-Base with TM1637

본 프로젝트는 ATmega328P의 내부 8비트 하드웨어 타이머(Timer/Counter0)를 활용하여 메인 루프의 간섭 없이 정확한 1초(1Hz) 타임 베이스를 생성합니다. 특히 외부 라이브러리에 의존하지 않고 데이터시트의 타이밍 차트를 직접 분석해, TM1637 디스플레이 모듈과의 정밀한 통신 및 동기화 시스템을 밑바닥부터 구현했습니다.

## 📌 Project Overview
단순한 `_delay_ms()` 함수 사용을 지양하고, MCU의 하드웨어 자원인 **CTC(Clear Timer on Compare Match) 모드**와 **Interrupt**를 활용하여 CPU 효율을 극대화했습니다. 

특히, 외부 라이브러리에 의존하지 않고 **TM1637 데이터시트의 전송 타이밍 규격(Interface Interpretation)**을 분석하여, CLK와 DIO 핀을 제어하는 2-Wire 직렬 통신 프로토콜을 소프트웨어(Bit-banging)로 직접 구현 및 연동했습니다.

## 🛠 Tech Stack & Environment
* **MCU:** ATmega328P (16MHz External Crystal)
* **Development:** Microchip Studio / AVR-GCC
* **Protocol:** GPIO (Bit-banging), Timer/Counter0 (CTC Mode)
* **Hardware:** TM1637 4-Digit 7-Segment Display Module

---

## ⚙️ Core Register Configuration
<img width="880" height="685" alt="image" src="https://github.com/user-attachments/assets/4762adae-5cbd-4483-bf7a-f9e183b1b5e2" />

본 프로젝트에서 타임 베이스(1ms 단위) 생성을 위해 조작한 핵심 레지스터 세팅 흐름입니다.

### 1. TCCR0A / TCCR0B (Timer/Counter Control Register)
* **`WGM01 = 1`**: 카운터가 특정 값에 도달하면 0으로 초기화되는 **CTC 모드** 활성화.
* **`CS01 = 1, CS00 = 1`**: 16MHz 시스템 클럭을 **64 분주(Prescaler)**하여 타이머 클럭(`clk_Tn`) 소스로 사용. (1클럭당 4µs)

### 2. OCR0A (Output Compare Register A)
* **`OCR0A = 249`**: 1ms 주기를 만들기 위한 비교 일치 기준값 설정. 
  *(4µs × 250 = 1000µs = 1ms. 0부터 카운트하므로 250 - 1 = 249)*

### 3. TIMSK0 (Timer/Counter Interrupt Mask Register)
* **`OCIE0A = 1`**: TCNT0 값과 OCR0A 값이 일치할 때(1ms 마다) 발생하는 하드웨어 **인터럽트를 허용(Enable)**.

### 4. SREG (Status Register)
* **`I-bit = 1 (sei())`**: 시스템 전역 인터럽트 활성화.

---

## 🔬 TM1637 데이터시트 기반 프로토콜 연동
<img width="767" height="211" alt="image" src="https://github.com/user-attachments/assets/0ebe5cf4-0721-4ba7-aeb9-a91833758f9e" />
<img width="977" height="311" alt="image" src="https://github.com/user-attachments/assets/6b0d5982-86ac-4b11-9100-a033a7fd7c44" />


TM1637 공식 데이터시트의 통신 규격을 바탕으로, 타이밍 마진(ns 단위까지 고려)을 고려한 소프트웨어 드라이버를 직접 설계했습니다.

* **Start/Stop Condition:** `CLK`이 High일 때 `DIO` 선의 에지 변화를 제어하여 통신의 시작과 종료 신호를 구현.
* **Data Transfer (Setup/Hold):** `CLK`이 Low일 때만 `DIO`의 비트 데이터를 변경하고, `CLK`이 High로 올라가는 상승 에지(Rising Edge)에서 TM1637 칩이 데이터를 안전하게 읽어가도록 정밀한 타이밍 딜레이(`_delay_us(5)`) 설계.
* **ACK (Acknowledge):** 8비트 데이터 전송 완료 후 9번째 클럭을 발생시켜 칩의 수신 확인 신호 구간을 하드웨어적으로 처리.

---

## 📖 System Logic Flow

1. **Initialize:** 타이머 및 TM1637 직렬 통신용 GPIO 핀 방향(DDR) 설정.
2. **Interrupt Trigger:** 백그라운드에서 하드웨어 타이머가 작동하며 **정확히 1ms마다** `TIMER0_COMPA_vect` ISR(인터럽트 서비스 루틴) 호출.
3. **Time Accumulation:** ISR 내부에서 카운터(`ms_count`)를 증가시키며, 1000회(1초) 누적 시 `time_updated` 플래그를 Set.
4. **Asynchronous Display:** 메인 루프(`while(1)`)는 평소에 대기하다가, 플래그가 켜지면 데이터시트 매핑 방식에 맞춰 TM1637 모듈로 숫자 데이터를 전송하고 화면을 갱신.
