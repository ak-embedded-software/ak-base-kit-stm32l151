<h1 align="center">Game Object Sequences</h1>

This document describes the runtime sequence and state management of each main object in **Lone-Blade**.

---

## I. Object Summary

| Object | File / Module | Main Responsibility |
|---|---|---|
| **Hero (Knight)** | `game_player.cpp` | Manages player position, HP (5 hearts), Mana (0-100%), Shield Parry state, and Ult Wave activation. |
| **Monsters** | `game_monster.cpp` | Spawns and moves Flying (+5 pts), Normal (+10 pts), and Armored Knights (+20 pts). |
| **Boss System** | `game_boss.cpp` | Handles Wave 3 (25 HP) & Wave 5 (40 HP) Bosses with Enraged Phase 2 when HP <= 50%. |
| **Arrow Hazard** | `game_arrow.cpp` | Spawns fast horizontal projectiles crossing screen. |
| **Ultimate Wave** | `game_ult_wave.cpp` | Piercing energy wave cutting across screen when Mana = 100%. |
| **Health Potion** | `game_item.cpp` | Drops random health pickup restoring +1 HP. |

---

## II. Detailed Object Sequences

### 1. Hero State Machine & Input Sequence

```mermaid
sequenceDiagram
    autonumber
    actor Player as Button Input
    participant Scr as Screen Handler
    participant Hero as Player Module

    Player->>Scr: Press [Up] / [Down]
    Scr->>Hero: player_attack(DIR_RIGHT / DIR_LEFT)
    Hero->>Hero: Set State ATTACK1/2, trigger slash animation

    Player->>Scr: Press [Mode]
    alt Mana == 100
        Scr->>Hero: player_ult_wave()
        Hero->>Hero: Reset Mana = 0, spawn Ult Wave
    else Mana < 100
        Scr->>Hero: player_shield(true)
        Hero->>Hero: Set State SHIELD, raise shield parry window
    end
```

---

### 2. Boss System Sequence (Wave 3 & Wave 5)

```mermaid
sequenceDiagram
    autonumber
    participant Engine as Game Loop
    participant Boss as Boss Module
    participant Hero as Player Module

    Engine->>Boss: game_boss_spawn(wave)
    Boss->>Boss: Set State SPAWNING (2.5s Alert Banner)
    Boss->>Boss: Set State IDLE

    loop Action Cooldown Loop
        Boss->>Boss: Select Attack (Charge / Slash / Fireball)
        alt Charge / Slash
            Boss->>Hero: Approach Player & Slash
            alt Player is SHIELD
                Hero-->>Boss: Parry Block (BUZZER_SOUND_CLICK)
            else Player is NOT SHIELD
                Boss->>Hero: Deal 2 Damage to Player
            end
        else Fireball
            Boss->>Boss: Spawn BossFireball
        end
    end
```

---

### 3. Combat Collision & Scoring Sequence

```mermaid
sequenceDiagram
    autonumber
    participant Hero as Hero
    participant Combat as Combat Engine
    participant Monster as Monster / Boss

    Monster->>Combat: Move into strike range
    Hero->>Combat: Attack / Parry
    alt Parry Success
        Combat->>Monster: Reflect Fireball / Block Slash
        Combat->>Hero: Gain Mana (+15)
    else Defeat Enemy
        Combat->>Monster: Reduce Enemy HP to 0
        Monster->>Combat: Add Score (Flying: +5, Normal: +10, Armored: +20, Boss: +300)
    end
```
