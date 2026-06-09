# ProteusManager


ProteusManager is a Qt/C++-based desktop application for the automated generation of database structures from class models using AI assistance.

## Project Description

The application analyzes class structures from different programming languages and automatically generates relational database structures as well as matching database interface classes.

In addition, a locally running AI via Ollama supports the analysis of class models and assists in generating and optimizing SQL tables and repository classes.

The goal of the project is to reduce manual effort during database development and improve the consistency and maintainability of database structures.

---

# Features

- Class structure analysis
- Automatic SQL generation
- SQLite support
- Database interface generation
- Local AI support via Ollama
- Qt-based desktop application
- Extendable architecture

---

# Technologies

## Programming Languages
- C++17
- SQL

## Frameworks / Libraries
- Qt 6
- SQLite
- Ollama API

## Development Environment
- Qt Creator
- CMake
- Git

---

# Planned Features

- Support for multiple programming languages
- Automatic relationship detection
- Repository generator
- CRUD code generation
- SQL preview
- AI-assisted database optimization
- Monitoring and tracking of generated database structure changes

---

# Local AI Setup

ProteusManager requires a reachable Ollama endpoint and at least one installed
AI model before SQL or DAL generation can run.

Setup instructions are available in:

- [docs/AI_SETUP.md](docs/AI_SETUP.md)

---

# System Requirements

- Windows 10 or higher
- Qt 6
- CMake
- Ollama

---

# Project Status

The project is currently under development.
