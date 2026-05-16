# ATmega328P-Real-Time-Clock-Base

- 본 프로젝트는 ATmega328P 내부 8비트 Timer/Counter 0을 정밀하게 제어하여 소프트웨어 루프의 간섭 엎이 정확한 1초 타임 베이스를 생성하고 이를 7-Segment에 표시하는 시스템을 구현했습니다.

### 흐름
- Clock Calculation
  - 16MHz 시스템 클럭을 1024 Prescaler로 분주하여 타이머 주파수를 하향 조정.
  - 8비트 타이머의 물리적 한계를 극복하기 위해 하드웨어 인터럽트(10ms)와 소프트웨어 카운터(x100)를 결합하여 정확한 1.000초를 구현.
- Register-Level Hardware Control
  - TCCR0A/B: 데이터시트 기반의 레지스터 직접 조작을 통해 CTC 모드 활성화.
  - OCR0A Mapping: 비교 일치 값을 156으로 설정하여 정밀한 인터럽트 주기 생성.
  - TIMSK0: OCIE0A 비트 활성화를 통한 비동기적 사건 처리.
  - 인터럽트 전역 허용 - sei()

### 회로도
<img width="837" height="663" alt="image (7)" src="https://github.com/user-attachments/assets/ad92808f-f30c-410a-81f5-c68d94a23939" />
