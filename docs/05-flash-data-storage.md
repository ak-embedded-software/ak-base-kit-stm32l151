<h1 align="center">Flash Data Storage Architecture</h1>

This document details the non-volatile data persistence implementation in **Lone-Blade** using STM32L151CBT6 Flash memory.

---

## I. Memory Partition Layout

The STM32L151CBT6 microcontroller features **128 KB** of Flash memory partitioned as follows:

```text
Flash Partitions Layout
----------------------
[ 0x08000000 - 0x08001FFF ] : Bootloader Partition (8 KB)
=> AK Bootloader

[ 0x08002000 - 0x08002FFF ] : BSF Shared Partition (4 KB)
=> Data sharing partition between Bootloader and Application

[ 0x08003000 - 0x0801FFFF ] : Application Firmware Partition (116 KB)
=> Contains Lone-Blade application firmware

[ Sector 0x6000 ]           : Dedicated Game Storage Sector (4 KB Sector)
=> Stores Magic Validation Number, High Score, and Top-5 Leaderboard
```

---

## II. Data Structure Schema

Non-volatile data is packed inside a fixed C++ structure:

```cpp
struct SaveData {
    uint32_t magic;          // Magic validation value: 0xABCD1234
    uint32_t high_score;     // Personal high score record
    uint32_t leaderboard[5]; // Top-5 high scores array (sorted descending)
};
```

---

## III. Read & Write Operations

### 1. On-Demand Reading (`game_load_flash_data`)
- Executed on booting or opening the Leaderboard menu.
- Reads `sizeof(SaveData)` bytes from Flash address `0x6000`.
- Validation check:
  - If `data.magic == 0xABCD1234`: Populates RAM arrays `g_leaderboard` and `g_high_score`.
  - If `data.magic` is invalid (fresh chip or uninitialized Flash): Initializes `high_score = 0` and `g_leaderboard[5] = {0, 0, 0, 0, 0}`.

### 2. High Score Update & Flash Erase/Write (`game_update_high_score`)
- Triggered on match completion (Win or Lose):
  1. Checks if `g_score` qualifies for Top-5 insertion.
  2. If ranking changes: Updates `g_leaderboard[5]` in RAM.
  3. Erases Flash Sector: `flash_erase_sector(0x6000)`.
  4. Writes struct: `flash_write(0x6000, (uint8_t*)&data, sizeof(data))`.

---

## IV. Score Persistence ("PLAY HARD" Continuation)

When a player defeats the Wave 5 Boss in **EASY** or **NORMAL** mode:
- The Victory screen displays **PLAY HARD**.
- If selected:
  - C-flag `g_keep_score_flag` is enabled (`game_set_keep_score_flag(true)`).
  - The new match begins in HARD mode without resetting `g_score` to 0, allowing the score to continue accumulating for new high-score records.
