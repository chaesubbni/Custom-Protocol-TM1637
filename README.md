# Custom-Protocol-TM1637

본 프로젝트는 외부 라이브러리에 전혀 의존하지 않고, ATmega328P MCU와 TM1637 모듈 간의 통신 프로토콜을 밑바닥부터 직접 구현한 프로젝트입니다. 단순히 코드를 짜맞추어 동작만 시키는 것에 그치지 않고, 데이터시트의 타이밍 차트, CPU의 명령어 처리 주기(ns), 내부 D-FF 래치 구조 그리고 외부 풀업 저항에 의한 물리적 RC 지연 시간(us)까지 정밀하게 계산하고 분석하여 us 단위에서 완벽한 동기화 제어 시스템을 구축했습니다.

## 📌 Project Overview
기존에 널리 쓰이는 외부 라이브러리나 아두이노 내장 함수를 일절 배제하고, 하드웨어의 물리적 특성을 완벽히 통제하며 2-Wire 직렬 통신(CLK, DIO)을 소프트웨어 방식(Bit-banging)으로 직접 설계했습니다. 

**1. 데이터시트 기반의 D-Flip Flop 타이밍 제어**
TM1637 내부의 D-FF(래치) 구조를 분석하여 데이터를 핀에 올리고 확정 짓는 타이밍을 정밀하게 제어합니다.
* **Setup Time (CLK Low):** D-FF의 입력문이 닫혀 있는 동안 DIO 전선의 전압을 변경하여, 데이터가 흔들림 없이 안착할 수 있는 안정화 시간을 확보합니다.
* **Latch (CLK Rising Edge):** CLK 전압이 올라가는 셔터 타이밍에 TM1637이 DIO 값을 정확하게 낚아채도록 동기화했습니다.
* **Hold Time (CLK High):** Start/Stop 조건 오작동을 막기 위해 CLK이 High인 구간에서는 DIO 상태를 고정하고 칩이 데이터를 소화할 시간을 제공합니다.

**2. RC 지연(Delay) 및 CPU 클럭 사이클 계산**
`_delay_us()`는 무작정 넣은 대기 시간이 아닙니다. 회로의 물리적 시상수(τ)와 CPU 명령어 처리 속도를 계산하여 도출한 **최적의 안전 마진(5us)**입니다.
* **CPU 처리 속도:** ATmega328P(16MHz)의 1주기는 62.5ns이며, 비트 제어 명령어(`CBI`, `SBI` 등 Read-Modify-Write)는 단 2주기(125ns)만에 실행이 완료됩니다.
* **RC 지연 극복:** 모듈의 10kΩ 풀업 저항과 100pF 커패시터(노이즈 필터)에 의한 시상수(τ)는 1us입니다. 핀을 놓았을 때(High) 5V까지 안정적으로 도달(95% 이상)하려면 최소 3us가 소요됩니다. CPU의 제어 속도가 전압 상승 속도보다 압도적으로 빠르기 때문에 발생하는 신호 꼬임을 방지하고자 5us의 의도적인 지연을 배치했습니다.

**3. Open-Drain 방식을 통한 과전류(Short Circuit) 보호**
MCU와 TM1637이 DIO 선의 제어권을 주고받을 때(특히 9번째 클럭 ACK 수신 시) 양측이 서로 다른 전압(5V와 0V)을 뿜어내어 칩이 타버리는 버스 충돌 현상을 원천 차단했습니다.
* **Low 출력:** `DDR`을 출력 모드로 설정하여 내부 GND 스위치를 닫고 전압을 0V로 강하게 끌어내립니다 (Sink).
* **High 출력:** `DDR`을 입력 모드로 전환하여 MCU는 선에서 완전히 손을 뗍니다(High-Z). 이후 외부 풀업 저항의 힘만으로 자연스럽게 5V가 채워지도록 유도하여 안전성을 극대화했습니다.

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
