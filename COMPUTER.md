
# Computer Specifics

This emulator comes with a basic computer built-in. You can program for this computer, or you can customize it.
The memory map can be changed by modifying the read() and write() functions in cpu.c

A basic BIOS for the computer is provived. The BIOS isn't done, but it is a good example for what the current GPU does.

## Computer specs

- 3 MiB of RAM
- 8 KiB BIOS/boot ROM
- Up to 2 128 KiB OS/data ROMs
- Ben Eater-inspired GPU with 512 KiB of VRAM - outputs 640x480 image (1 byte/pixel; --BBGGRR)
- 4 Expansion ports (not sure what these would be used for yet, or if they should even exist; right now the idea is a GPIO header with the CPU's address bus, data bus, interrupt pins, etc. with a /CE signal for memory mapping)

## CPU Memory Map

- $000000 - $007fff: Low RAM (Mirror of WRAM $100000-$107fff)
- $008000 - $00dfff: Hardware Registers & I/O
  - $8000 - $8fff: SD Card (?)
  - $9000 - $9fff: 6522 VIA
  - $a000 - $afff: PS/2 Keyboard
    - $a000-$a7ff: Scancode
    - $a800-$afff: Interrupt Flag
  - $b000 - $bfff: PS/2 Mouse (?)
  - $c000 - $cfff: GPU Control
  - $d000 - $dfff: EXP IDs
- $00e000 - $00ffff: BIOS / Boot ROM (8 KiB)
- $080000 - $0fffff: VRAM (512 KiB)

- $100000 - $1fffff: WRAM 0 (1 MiB)
- $200000 - $2fffff: WRAM 1 (1 MiB) (Optional)
- $300000 - $3fffff: WRAM 2 (1 MiB) (Optional)

- $400000 - $4fffff: EXP 0 (1 MiB)
- $500000 - $5fffff: EXP 1 (1 MiB)
- $600000 - $6fffff: EXP 2 (1 MiB)
- $700000 - $7fffff: EXP 3 (1 MiB)

- $fc0000 - $fdffff: OS ROM 1 (128 KiB)
- $fe0000 - $ffffff: OS ROM 0 (128 KiB)
