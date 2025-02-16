# 🌀 OS Internals & Simulation LLD Projects

This repository contains **low-level system design** and **simulations of OS internals**. These projects focus on **core Operating System concepts** such as **memory management, scheduling, I/O handling, and linking & loading**. They emphasize **performance optimizations, low-level debugging, and efficient resource management**—critical skills in **Systems Programming and OS Development**. 💡

<p align="center">
  <img src="https://litslink.com/wp-content/uploads/2023/01/What-Is-C-Comparative-Advantages_%D0%9C%D0%BE%D0%BD%D1%82%D0%B0%D0%B6%D0%BD%D0%B0%D1%8F-%D0%BE%D0%B1%D0%BB%D0%B0%D1%81%D1%82%D1%8C-1.png" width="400" alt="Racket Logo">
</p>

## 🏗️ Projects Breakdown

### 1️⃣ **Linker & Loader Simulation**
- Simulated a **two-pass linker and loader**, handling **symbol resolution, relocation, and memory management**.
- Implemented:
  - **Symbol Table Construction** 📜
  - **Memory Relocation Handling** 🏗️
  - **Error Detection & Warnings** ⚠️
  - **Instruction Parsing & Processing** 💾
  - **Tokenization & Lexical Analysis** 🔍

### 2️⃣ **CPU Process Scheduling Discrete Event Simulator**
- Developed a **CPU scheduler simulation** supporting **multiple scheduling algorithms**:
  - **First Come First Serve (FCFS)** ⏳
  - **Last Come First Serve (LCFS)** 🔄
  - **Shortest Remaining Time First (SRTF)** 🏃‍♂️
  - **Round Robin (RR)** 🔁
  - **Priority Scheduling (PRIO & PREPRIO)** 📊
- Implemented **event-driven simulation** with preemptive and non-preemptive scheduling.

### 3️⃣ **Memory Management Unit (MMU) Simulation**
- Built a **page replacement simulation**, replicating the functionality of an MMU with different replacement strategies:
  - **FIFO (First-In-First-Out)** 📦
  - **Random Replacement (RAND)** 🎲
  - **Clock Algorithm (CLOCK)** 🕰️
  - **Enhanced Second-Chance (ESC)** 🔄
  - **Aging Algorithm** 👴
  - **Working Set Clock (WS-Clock)** 🏁
- Implemented **frame allocation policies**, **TLB simulation**, and **page fault handling**.

### 4️⃣ **Disk I/O Scheduling Simulation**
- Simulated **disk I/O scheduling algorithms**, including:
  - **First Come First Serve (FIFO)** 📜
  - **Shortest Seek Time First (SSTF)** 🏎️
  - **LOOK & C-LOOK (Circular LOOK)** 🔄
  - **FLOOK (Flexible LOOK)** 📈
- Managed **seek time optimization**, **queueing mechanisms**, and **request prioritization**.

## 🚀 Running the Simulations

### 📌 Prerequisites
- **C++ Compiler** (g++ recommended)
- **Makefile** (for easy compilation)

### ▶️ Execution
Compile and run any project with:
```bash
g++ -o <executable> <source-file>.cpp
./<executable> <input-file> [optional flags]
```

## 📚 Key OS Concepts Covered
- **Process Scheduling & Context Switching** 🔄
- **Memory Management & Page Replacement Policies** 📦
- **Virtual Memory & Frame Allocation Strategies** 🧠
- **Linking & Loading Mechanisms** 🔗
- **File System & Disk I/O Handling** 💽

## 📁 Repository Structure
```
├── linker.cpp             # Linker & Loader Simulation
├── sched.cpp          # CPU Process Scheduling
├── mmu.cpp                # Memory Management Unit (MMU)
├── iosched.cpp       # Disk I/O Scheduling
├── README.md              # Project Documentation 📜
```

These projects serve as a **solid foundation for system-level software engineering** and **low-level OS programming**. 🛠️ If you're interested in hiring me for **Kernel Development, C++ SWE, Storage Systems, or Virtual Memory Management**, these simulations will be highly relevant! 🚀
