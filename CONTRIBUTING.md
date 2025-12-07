# Contributing to ChaosOfSymbols

Thank you for considering contributing! This document will guide you through the process.

## How to Contribute?

### Reporting Bugs
1. **Search existing issues** — check if the bug is already reported
2. **Use the bug report template** when creating a new issue
3. **Provide details**:
   - Clear description
   - Steps to reproduce
   - Expected vs actual behavior
   - Screenshots if applicable
   - Environment (OS, browser, version)

### Suggesting Features
1. **Check if feature already exists** in issues or discussions
2. **Use the feature request template**
3. **Explain**:
   - The problem this solves
   - Proposed solution
   - Alternatives considered

## Development Setup

### Prerequisites
Visual Studio 2022 (recommended) or 2019
Git for version control
C++ Compiler supporting at least C++17

### Installation with Visual Studio

1. **Clone the repository**
git clone https://github.com/WirrWarr-rgb/ChaosOfSymbols.git
cd WirrWarr-rgb/ChaosOfSymbols

2. **Open the solution file (ChaosOfSymbols.sln or similar) in Visual Studio**
npm install

3. **Build the project**
   - Select configuration (Debug/Release)
   - Build Solution (F7 or Ctrl+Shift+B)

4. **Run the game**
   - Set the game project as Startup Project
   - Run with F5 (Debug) or Ctrl+F5 (Start Without Debugging)

### Git Commit Messages
- Use [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/)
- Format: `<type>(<scope>): <description>`
- Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

**Examples:**
- feat: refactor save system with templates and help panel
- docs: add CODE_OF_CONDUCT.md

### Branch Naming
- `feature/` - new features
- `bugfix/` - bug fixes  
- `hotfix/` - urgent fixes
- `docs/` - documentation
