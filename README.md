# ATmega328P-Real-Time-Clock-Base

본 프로젝트는 **ATmega328P**의 내부 8비트 하드웨어 타이머(**Timer/Counter0**)를 제어하여, 소프트웨어 루프의 간섭 없이 정확한 **1초(1Hz)** 타임 베이스를 생성하고 이를 디지털 디스플레이(TM1637)에 표시하는 시스템을 구현합니다.
데이터시트의 타이밍 차트를 분석해 직접 구현했습니다.

## 📌 Project Overview
단순한 `_delay_ms()` 함수 사용을 지양하고, MCU의 하드웨어 자원인 **CTC(Clear Timer on Compare Match) 모드**와 **Interrupt**를 활용하여 CPU 효율을 극대화했습니다. 추가로, 외부 라이브러리에 의존하지 않고 TM1637 디스플레이 제어용 2-Wire 직렬 통신 프로토콜을 소프트웨어로 직접 구현했습니다.

## 🛠 Tech Stack & Environment
* **MCU:** ATmega328P (16MHz External Crystal)
* **Development:** Microchip Studio 
* **Protocol:** GPIO, Timer/Counter0 (CTC Mode)
* **Hardware:** TM1637 4-Digit 7-Segment Display Module

---

## Timer/Counter Core Register Configuration

본 프로젝트에서 타임 베이스(1ms 단위) 생성을 위해 조작한 핵심 레지스터 세팅 흐름입니다.
<img width="837" height="663" alt="image (7)" src="https://github.com/user-attachments/assets/bde96929-d2ce-4379-a454-bd5fb53daedf" />

### 1. TCCR0A / TCCR0B (Timer/Counter Control Register)
* **`WGM01 = 1`**: 카운터가 특정 값에 도달하면 0으로 초기화되는 **CTC 모드** 활성화.
* **`CS01 = 1, CS00 = 1`**: 16MHz 시스템 클럭을 **64 분주(Prescaler)**하여 타이머 클럭으로 사용. (1클럭당 4µs)

### 2. OCR0A (Output Compare Register A)
* **`OCR0A = 249`**: 1ms 주기를 만들기 위한 비교 일치 기준값 설정. 
  *(4µs × 250 = 1000µs = 1ms. 0부터 카운트하므로 250 - 1 = 249)*

### 3. TIMSK0 (Timer/Counter Interrupt Mask Register)
* **`OCIE0A = 1`**: TCNT0 값과 OCR0A 값이 일치할 때(1ms 마다) 발생하는 하드웨어 **인터럽트를 허용(Enable)**.

### 4. SREG (Status Register)
* **`I-bit = 1 (sei())`**: 시스템 전역 인터럽트 활성화.

## TM1637 통신 프로토콜
<img width="866" height="306" alt="image" src="https://github.com/user-attachments/assets/fb11e876-fffa-4433-9751-e566f851d1a9" />


## 📖 System Logic Flow

1. **Initialize:** 타이머 및 직렬 통신용 GPIO 핀 방향(DDR) 설정.
2. **Interrupt Trigger:** 백그라운드에서 하드웨어 타이머가 작동하며 **정확히 1ms마다** `TIMER0_COMPA_vect` ISR(인터럽트 서비스 루틴) 호출.
3. **Time Accumulation:** ISR 내부에서 카운터(`ms_count`)를 증가시키며, 1000회(1초) 누적 시 `time_updated` 플래그를 Set.
4. **Asynchronous Display:** 메인 루프(`while(1)`)는 평소에 대기하다가, 플래그가 켜지면 TM1637 모듈과 통신하여 화면의 숫자를 갱신.

## 타이밍 분석
- Atmega328P는 16MHz로 동작해 1개의 명령을 처리하는데 0.0625us 밖에 걸리지 앟음.
- TM1637 디스플레이 칩은 최대 통신 속도가 약 250KHz~400KHz로 신호를 받아들이는데 2us ~ 4us가 필요함.
- 그래서 중간중간에 _delay_us(5)로 타이밍을 맞춰줌.
> 🔗 **Deep Dive:** 내부 클럭 동기화(MUX 제어) 및 플래그 D-FF 업데이트 타이밍에 대한 상세한 하드웨어 분석은 [개인 기술 블로그 링크 삽입 예정]에서 확인하실 수 있습니다.
