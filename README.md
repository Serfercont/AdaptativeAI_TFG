# Adaptive Hostile AI — UE5 Bachelor's Thesis

> A real-time adaptive hostile AI system for Unreal Engine 5, built as a Bachelor's Final Project (TFG) at CITM-UPC.
> Designed to demonstrate that indie studios can implement AAA-quality adaptive AI with limited resources.

![Made with Unreal Engine 5](https://img.shields.io/badge/Unreal%20Engine-5-313131?style=flat&logo=unrealengine)
![Language C++](https://img.shields.io/badge/Language-C%2B%2B-00599C?style=flat&logo=c%2B%2B)
![License Educational](https://img.shields.io/badge/License-Educational-blue)
![Status Demo](https://img.shields.io/badge/Status-Playable%20Demo-success)

---

## Table of Contents

- [About the Project](#about-the-project)
- [Key Features](#key-features)
- [Enemies & Behaviors](#enemies--behaviors)
- [How to Play](#how-to-play)
- [Controls](#controls)
- [System Architecture](#system-architecture)
- [Build & Run](#build--run)
- [Validation Results](#validation-results)
- [Project Status & Roadmap](#project-status--roadmap)
- [Author](#author)
- [License](#license)

---

## About the Project

This project explores how to build a **real-time adaptive hostile AI** that reacts to the player's playstyle, in a context where most indie studios lack the resources to implement such systems. The goal is not to ship a complete game, but to deliver a **reusable, well-documented AI framework** that other indie developers can study, fork, and integrate into their own projects.

The system is built on a **hybrid architecture** combining four AI techniques:

- **Behavior Trees** for hierarchical decision-making
- **Utility AI** for adaptive action weighting via mathematical curves
- **Environment Query System (EQS)** for tactical position scoring
- **Squad Brain** for collective strategy detection and coordination

All code is written in **C++** with light support from **Blueprints**, targeting **Unreal Engine 5**.

---

## Key Features

- **Two fully implemented enemy types** with distinct AI architectures
- **Squad coordination** with player strategy detection (Aggressive / Camping / Silent)
- **Utility AI Component** as a reusable `UActorComponent` with 5 curve types
- **Dynamic role assignment** within squads (Sniper / Rifle / Shotgun)
- **Collective tactics** including flanking, suppression fire, and defensive retreat
- **Per-role specialized behaviors** with full Utility AI integration for the Rifle role
- **Adaptive profiles** that modulate enemy decisions based on detected player strategy
- **Stress saturation** preventing AI from being unbeatable
- **Empirically validated** with a 7-participant qualitative study

---

## Enemies & Behaviors

### Infected (Zombie-like)

Individual instinct-driven enemy that operates without coordination but propagates information through screams.

| Behavior | Description |
|---|---|
| **Patrol** | Default state when no player is detected. |
| **The Call (*La Crida*)** | Once player is detected, alerts nearby allies within range. |
| **Stone Throw** | When the player is unreachable (e.g. on elevation), the infected throws stones with predictive arc. |
| **Final Jump Attack** | When health drops below 20%, the infected leaps toward the player with velocity prediction. |
| **Flee** | When low health combined with low ally count, retreats from combat. |
| **Dodge** | Zig-zag movement during pursuit to avoid ranged fire. |
| **Fury Mode** | Below 20% HP, gains speed and damage multipliers. |

### Mercenary (Human Squads)

Coordinated tactical enemies organized in 5-member squads with role specialization and a Squad Brain that detects player strategy.

| Role | Behavior |
|---|---|
| **Sniper** | Long-range engagement (8000 units). Excluded from flanking. Repositions to elevated cover. |
| **Rifle** | Mid-range, polyvalent. Full Utility AI with 4 weighted actions: Shoot, Reload, Cover, Reposition. Shoots while repositioning. |
| **Shotgun** | Aggressive kamikaze charge (450 walk speed × 1.5 multiplier). Charges until close range. |

**Collective tactics triggered by the Squad Brain:**

| Tactic | Trigger | Behavior |
|---|---|---|
| **Flank + Suppression** | Player stays still > 5 seconds (camping detection) | 35% of squad flanks while the rest provides suppression fire. |
| **Defensive Retreat** | Player kills ≥ 5 mercenaries within 15 seconds (aggressive detection) | Active members break engagement and seek cover. Snipers excluded. |
| **Information Sharing** | Any member sees the player | Position propagated to all squad members through the Blackboard. |

---

## How to Play

This is a **playable test field** designed to showcase the AI system. Objectives:

1. Move around the open area with scattered obstacles
2. Engage both infected and mercenary squads
3. Try different playstyles (camping behind cover vs. aggressive rushing) and observe how the AI adapts
4. Switch tactics mid-fight to see real-time adaptation

> **Tip:** The most impressive behavior happens when you **change playstyle mid-combat**. Stay still for 5+ seconds to trigger flanking, or kill mercenaries quickly to trigger defensive retreats.

---

## Controls

| Action | Key / Input |
|---|---|
| Move | **W / A / S / D** |
| Look around | **Mouse** |
| Jump | **Space** |
| Shoot | **Left Mouse Button** |
| Reload | **Right Mouse Button** |

---

## System Architecture
```
┌─────────────────────────────────────────────────────┐
│              SQUAD BRAIN (EnemySquad)               │
│  • Detects player strategy (Aggressive/Camping)     │
│  • Triggers collective tactics                      │
│  • Coordinates roles between squad members          │
└────────────────────┬────────────────────────────────┘
                     │ propagates strategy
                     ▼
┌─────────────────────────────────────────────────────┐
│        INDIVIDUAL ADAPTIVE PROFILE                  │
│  • UpdateAdaptativeProfile()                        │
│  • Modulates Utility AI multipliers per profile     │
└────────────────────┬────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────┐
│         UTILITY AI COMPONENT                        │
│  • Evaluates weighted actions every 0.5s            │
│  • Curves: Linear, Exponential, Logistic,           │
│           Inverse, InverseExponential               │
│  • Writes boolean "ShouldX" keys to Blackboard      │
└────────────────────┬────────────────────────────────┘
                     │ ShouldFlee, ShouldThrow, etc.
                     ▼
┌─────────────────────────────────────────────────────┐
│         BEHAVIOR TREE + EQS                         │
│  • Consumes Should* booleans as branch entry        │
│  • Executes specific behavior tasks                 │
│  • EQS provides tactical positions                  │
└─────────────────────────────────────────────────────┘
```
### Key Source Files

| File | Responsibility |
|---|---|
| `UtilityAIComponent.cpp/h` | Reusable Utility AI component with 5 curve types |
| `UtilityCurves.h` | Mathematical curve implementations |
| `EnemyMercenary.cpp/h` | Mercenary pawn, weapon handling, role-specific config |
| `MercenaryAIController.cpp/h` | Perception, two-phase target forgetting |
| `EnemySquad.cpp/h` | Squad Brain: player strategy detection, tactic coordination |
| `SquadManager.cpp/h` | Squad formation and role distribution |
| `AEnemyInfected.cpp/h` | Infected pawn with Utility AI and global damage memory |
| `BaseAIController.cpp/h` | Base AI controller with perception setup |

---

## Author

**Sergio Fernandez**
Bachelor's Degree in Video Game Design and Development
CITM - UPC
2026

---
