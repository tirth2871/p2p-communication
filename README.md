# STM32 P2P RS-485 Communication

Peer-to-peer communication between two STM32F407G-DISC1 boards over RS-485.

## Hardware
- 2× STM32F407G-DISC1
- 2× MAX485 module
- Jumper wires, breadboard

## Wiring
| STM32 Pin | MAX485 Pin | Notes              |
|-----------|------------|--------------------|
| PA2       | DI         | USART2 TX          |
| PA3       | RO         | USART2 RX          |
| PA8       | DE + RE    | Direction control  |
| 3.3V      | VCC        |                    |
| GND       | GND        | Shared across both boards |

Connect A↔A and B↔B between the two MAX485 modules.

## Build
Requires `arm-none-eabi-gcc` and `make`.

```bash
# Flash Board A (Node 1 — sender)
make NODE_ID=1

# Flash Board B (Node 2 — receiver)
make NODE_ID=2
```

Flash the `.elf` file from `build/` using STM32CubeProgrammer.

## Phase Status
- ✅ Phase 1: Basic polling UART over RS-485
- ✅ Phase 2: Interrupt-driven RX, ring buffer, ACK handshake
- 🔲 Phase 3: Packet structure, CRC, reliability