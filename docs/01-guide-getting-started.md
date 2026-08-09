<h1 align="center">Game Programming Getting Started Guide</h1>

Welcome to the **Lone-Blade** game development guide on the STM32L151 microcontroller! This repository provides the complete source code base along with detailed documentation to help you understand the system architecture.

---

## Table of Contents

- [I. Create Your Own "Playground" (Fork)](#i-create-your-own-playground-fork)
- [II. Quick Start Guide (Environment Setup)](#ii-quick-start-guide-environment-setup)
- [III. Game Programming Workflow](#iii-game-programming-workflow)
  - [Step 1: Create your working directory](#step-1-create-your-working-directory)
  - [Step 2: Clone the repo to your machine](#step-2-clone-the-repo-to-your-machine)
  - [Step 3: Modify & Build Firmware](#step-3-modify--build-firmware)
  - [Step 4: Push your code to GitHub](#step-4-push-your-code-to-github)

---

## I. Create Your Own "Playground" (Fork)

To initialize your personal project, follow these steps:

### 1. Access the repository
Link repository: `https://github.com/the-ak-foundation/ak-base-kit-stm32l151`

### 2. Fork the repository
Click the **Fork** button in the top-right corner to create a copy of the project under your personal account.

---

## II. Quick Start Guide (Environment Setup)

To build the source code and flash firmware onto the kit, set up the development environment on Ubuntu/Linux:

- **ARM GCC Toolchain:** `gcc-arm-none-eabi` version 10.3-2021.07.
- **GNU Make:** `make` utility.
- **Flashing Utility:** `stm32flash` or `st-link`.

---

## III. Game Programming Workflow

### Step 1: Create your working directory

From your `Home` directory, create a folder named `Workspace` holding `Sources` and `Tools`:

```bash
mkdir -p ~/Workspace/Sources
mkdir -p ~/Workspace/Tools
```

### Step 2: Clone the repo to your machine

```bash
cd ~/Workspace/Sources
git clone https://github.com/<your-username>/lone-blade-main.git
```

### Step 3: Modify & Build Firmware

Navigate to the `application` folder to compile:
```bash
cd ~/Workspace/Sources/lone-blade-main/application
make clean && make
```

Flash firmware directly over USB/UART:
```bash
make flash dev=/dev/ttyUSB0
```

### Step 4: Push your code to GitHub

```bash
git add .
git commit -m "Update gameplay features"
git push origin main
```
