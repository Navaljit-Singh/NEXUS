#!/usr/bin/env bash
# setup_wsl.sh — Bootstrap the NEXUS development environment inside WSL (Ubuntu 24.04).
# Run once after cloning the repository:
#   bash /mnt/d/NEXUS/scripts/setup_wsl.sh
set -euo pipefail

echo "=== NEXUS Development Environment Setup ==="

# ------------------------------------------------------------------
# System packages: C++ toolchain, build system, and library headers
# ------------------------------------------------------------------
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    clang \
    clang-format \
    clang-tidy \
    libssl-dev \
    libboost-all-dev \
    libprotobuf-dev \
    protobuf-compiler \
    libpqxx-dev \
    postgresql-client \
    python3-pip \
    python3-venv

echo "=== Installing Python research dependencies ==="

# ------------------------------------------------------------------
# Create a virtual environment and install Python research packages.
# The venv is created inside d:\NEXUS\python so it stays with the
# project but is excluded from version control (.gitignore: .venv/).
# ------------------------------------------------------------------
cd /mnt/d/NEXUS/python \
    && python3 -m venv .venv \
    && source .venv/bin/activate \
    && pip install --upgrade pip \
    && pip install -r requirements.txt

echo "=== Setup complete. Build with: cd /mnt/d/NEXUS && cmake --preset release && cmake --build --preset release ==="
