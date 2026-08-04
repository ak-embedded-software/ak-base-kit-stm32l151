# Dungeon Loop - Embedded Turn-Based Adventure

## Overview

Dungeon Loop is a lightweight turn-based RPG adventure designed for embedded systems using:

* STM32L151 MCU
* SSD1306 OLED Display
* Winbond W25Q80 External Flash

The game focuses on:

* modular architecture
* embedded runtime systems
* save/load mechanics
* event-driven gameplay
* low-resource rendering
* strategic turn-based combat

The player travels through dangerous forest paths while collecting items, fighting monsters, and surviving through multiple stages.

---

# Gameplay Flow

```text
BOOT
 ↓
MAIN MENU
 ↓
SELECT LEVEL
 ↓
START STAGE
 ↓
AUTO MOVE
 ↓
EVENT TRIGGER
 ↓
BATTLE / ITEM / TRAP
 ↓
REWARD
 ↓
NEXT STAGE
 ↓
LEVEL COMPLETE
 ↓
SAVE GAME
```

---

# Main Menu

```text
> Continue
  New Game
  Select Level
  Settings
```

---

# Pause Menu

```text
> Resume
  Save Game
  Exit To Menu
```

---

# Core Gameplay

The player automatically travels through a dangerous forest path.

During the journey, random events may occur:

* Monster Encounter 👾
* Mystery Chest ❓
* Trap 💀
* Healing Shrine ❤️
* Boss Gate 🐉

The player must survive through all stages in a level.

The game is designed around:

* resource management
* turn planning
* item usage
* status effect control
* risk vs reward decisions

---

# Level System

| Level | Difficulty | Stages | Enemies |
| :--- | :---: | :---: | :--- |
| 1 | Easy | 4 | 1 Slime - 1 Goblin - 1 Wolf - 1 Gorilla |
| 2 | Easy | 5 | 2 Slime - 1 Goblin - 1 Wolf - 1 Gorilla |
| 3 | Medium | 6 | 1 Slime - 2 Goblin - 1 Wolf - 1 Gorilla - 1 Dragon |
| 4 | Medium | 7 | 1 Slime - 2 Goblin - 2 Wolf - 1 Gorilla - 1 Dragon |
| 5 | Hard | 8 | 1 Slime - 2 Goblin - 1 Wolf - 2 Gorilla - 1 Dragon - 1 Eye Watcher |

---

# Player Base Stats

| Stat | Base Value | Description |
| :--- | :---: | :--- |
| HP | 100 | Player Health |
| ATK | 10 | Base Attack Damage |
| DEF | 5 | Damage Reduction |
| Inventory | 6 Slots | Item Storage |
| Status Effect | N/A | Poison / Burn / Curse |

---

# Combat Formula

## Damage Formula

```text
Final Damage = ATK - (DEF / 2)
```

Minimum damage is always:

```text
1 Damage
```

Example:

```text
Enemy ATK = 20
Player DEF = 10

Final Damage:
20 - (10 / 2)
= 15 Damage
```

---

# Defend Action

When the player uses Defend:

```text
Incoming Damage Reduced by 50%
```

Defend only lasts for 1 turn.

---

# Mystery Event System

When the player encounters a mystery event:

```text
You found a mysterious chest...
Choose one item:
```

Example:

```text
1. Sword (+5 ATK)
2. Shield (+5 DEF)
3. Healing Potion (+25 HP)
```

The player can only choose one item.

---

# Item System

| Item | Description | Base Effect | Scaling | Spawn Level |
| :--- | :--- | :---: | :---: | :---: |
| Poison Bottle | Deals DOT damage to enemies | 8 Damage Over Time | +2 DOT / Level | 1 |
| Antidote | Removes Poison effect | Instant Cure | N/A | 1 |
| Purify | Removes Curse and Burn | Full Cleanse | N/A | 4 |
| Healing Potion | Restores HP | +25 HP | +10 HP / Level | 1 |
| Sword | Increases player ATK | +5 ATK | +2 ATK / Level | 1 |
| Shield | Increases player DEF | +4 DEF | +2 DEF / Level | 1 |
| Bomb | Reduces enemy armor | -5 DEF | +2 DEF Reduction / Level | 1 |

---

# Healing Scaling

| Level | Potion Healing |
| :--- | :---: |
| 1 | 25 HP |
| 2 | 35 HP |
| 3 | 45 HP |
| 4 | 55 HP |
| 5 | 70 HP |

---

# Trap Event

Example:

```text
A hidden trap was triggered!
-10 HP
```

Or:

```text
You are poisoned!
Poison damage activated.
```

Possible trap effects:

* Direct damage
* Poison
* Burn
* Temporary DEF reduction

---

# Monster Encounter

```text
A wild Goblin appeared!
```

The game enters battle mode.

---

# Monster Balance Table

| Monster | HP | DMG | Special Ability | Cooldown | Scaling | Spawn Level |
| :--- | :---: | :---: | :--- | :---: | :--- | :---: |
| Slime | 30 | 5 | Regenerates HP | Every 3 Turns | +5 HP / Level | 1 -> 5 |
| Goblin | 50 | 10 | Applies Poison | Every 4 Turns | +2 DMG / Level | 1 -> 5 |
| Wolf | 70 | 18 | Dodges next attack | Every 4 Turns | +3 HP / Level | 1 -> 5 |
| Gorilla | 100 | 22 | Gains Armor | Every 3 Turns | +3 DEF / Level | 1 -> 5 |
| Dragon | 150 | 30 | Burn Damage | Every 4 Turns | +5 HP / Level | 3 -> 5 |
| Eye Watcher | 200 | 35 | Curse Healing | Every 3 Turns | +5 HP / Level | 5 |

---

# Monster Ability Design

## Slime

Ability:

* Periodically regenerates HP

```text
Slime regenerates HP!
+5 HP
```

Designed as:

* beginner enemy
* sustain mechanic tutorial

---

## Goblin

Ability:

* Applies poison damage over time

```text
Goblin poisoned the player!
```

Poison Damage:

```text
5 Damage per turn
Duration: 3 Turns
```

Designed as:

* pressure enemy
* resource drain enemy

---

## Wolf

Ability:

* Dodges the next incoming attack

```text
Wolf dodged the attack!
MISS
```

Cooldown:

```text
Every 4 Turns
```

Designed as:

* agility enemy
* anti-burst enemy

---

## Gorilla

Ability:

* Gains temporary armor

```text
Gorilla hardened its armor!
+5 DEF
```

Designed as:

* tank enemy
* anti-physical enemy

---

## Dragon

Ability:

* Applies burn damage

```text
Dragon breathes fire!
Burn activated.
```

Burn Effect:

```text
10 Total Damage
Duration: 3 Turns
```

Designed as:

* boss-type pressure enemy
* sustained damage enemy

---

## Eye Watcher

Ability:

* Reduces healing effectiveness

```text
Eye Watcher cursed the player!
Healing reduced by 30%.
```

Designed as:

* anti-heal enemy
* late-game control enemy

---

# Status Effects

| Effect | Description |
| :--- | :--- |
| Poison | Damage every turn |
| Burn | Small DOT damage |
| Curse | Reduces healing effectiveness |
| Armor Break | Reduces DEF |

---

# Battle System

## Battle Screen

```text
PLAYER
HP: 100
ATK: 10
DEF: 5

VS

GOBLIN
HP: 50
DMG: 10
```

---

# Player Actions

The player has 5 main actions:

| Action | Description |
| :--- | :--- |
| Attack | Standard attack |
| Item | Use item |
| Defend | Reduce incoming damage |
| Skill | Use special ability |
| Escape | Attempt to flee |

---

# Battle Flow

```text
Player Turn
 ↓
Enemy Turn
 ↓
Status Effect Update
 ↓
Next Turn
```

---

# Difficulty Philosophy

The game balance is designed around:

* steady progression
* low RNG frustration
* strategic item usage
* manageable damage scaling
* short but meaningful battles

The game avoids:

* instant one-shot damage
* unfair dodge spam
* infinite healing loops
* defense becoming invincible

---

# Win Condition

The player wins by defeating all monsters in the final stage.

```text
LEVEL COMPLETE
```

---

# Lose Condition

The player loses when HP reaches zero.

```text
GAME OVER
```

---

# Save System

The game stores:

* Current Level
* Current Stage
* Player HP
* Inventory
* Status Effects
* Score

Save data is stored inside External Flash memory.

---

# Continue System

When selecting Continue:

```text
Loading Save Data...
Resume Adventure
```

The game restores:

* player progress
* inventory
* stage state

---

# Embedded Design Philosophy

This project is designed specifically for embedded hardware limitations.

Focus Areas:

* low memory usage
* low rendering overhead
* event-driven architecture
* modular game runtime
* bare-metal execution

Avoided Features:

* heavy animation
* realtime physics
* high FPS rendering
* complex graphics pipeline

---

# Embedded Architecture Goals

The project demonstrates:

* External Flash management
* Bootloader system
* Metadata handling
* Save/Load partition
* Dynamic game loading
* Modular Game API
* Bare-metal game runtime

Target Platform:

* STM32L151
* SSD1306 OLED
* W25Q80 SPI Flash

