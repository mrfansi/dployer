# Dployer

`Dployer` is a C-based deployment tool designed to manage and deploy repositories with Docker. The project is structured for easy configuration and flexibility, supporting both macOS and Linux environments.

## Table of Contents
- [Dployer](#dployer)
  - [Table of Contents](#table-of-contents)
  - [Installation](#installation)
    - [Prerequisites](#prerequisites)
    - [Building from Source](#building-from-source)
  - [Configuration](#configuration)
  - [Usage](#usage)
  - [Commands](#commands)
  - [Development](#development)
    - [Project Structure](#project-structure)
    - [Adding New Features](#adding-new-features)
    - [Building and Running](#building-and-running)
  - [License](#license)
    - [Summary of Changes:](#summary-of-changes)

## Installation

### Prerequisites
Ensure that you have the following installed:
- CMake (version 3.15 or higher)
- SQLite3
- JSON-C library
- Docker
- Git

### Building from Source

1. Clone the repository:

   ```bash
   git clone https://github.com/mrfansi/dployer.git
   cd dployer
   ```

2. Build using CMake:

   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

3. Install the binary and configuration files to `{HOME}/.config/dployer`:

   ```bash
   make install
   ```

## Configuration

By default, `Dployer` uses configuration files located in `{HOME}/.config/dployer/config`. The directory should contain subdirectories like `laravel` and `static-php` that hold the necessary Docker configuration files.

Example structure:
```
~/.config/dployer/
├── config/
│   ├── laravel/
│   │   └── Dockerfile
│   └── static-php/
│       └── Dockerfile
└── repositories/
```

## Usage

After building and installing, you can run `Dployer` from the terminal:

```bash
~/.config/dployer/dployer
```

This will launch an interactive mini terminal where you can run various commands to manage your repositories.

## Commands

- `new` - Create a new repository entry.
- `list` - List all repositories.
- `update` - Update all repositories.
- `update <ID>` - Update a specific repository by ID.
- `switch <ID> <BRANCH_OR_TAG>` - Switch to a specific branch or tag for a repository.
- `deploy` - Deploy all repositories.
- `deploy <ID>` - Deploy a specific repository by ID.
- `exit`, `quit` - Exit the mini terminal.
- `help` - Show the help message.

## Development

### Project Structure

- **src/**: Source code files.
  - `main.c`: The main entry point.
  - `logger.c` / `logger.h`: Handles logging.
  - `database.c` / `database.h`: Manages database interactions.
  - `repo.c` / `repo.h`: Handles repository management.
  - `deploy.c` / `deploy.h`: Manages deployment processes.
  - `docker.c` / `docker.h`: Docker-related operations.
  - `utils.c` / `utils.h`: Utility functions.

### Adding New Features

To add new features:
1. Create or modify source files in the `src/` directory.
2. Include the appropriate headers in your new files.
3. Update `CMakeLists.txt` to include new source files.

### Building and Running

To rebuild the project after making changes:

```bash
cd build
make
```

Then, run the `dployer` binary as usual:

```bash
~/.config/dployer/dployer
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

### Summary of Changes:

1. **Installation Section**:
   - Clarified installation steps, including how to build using CMake.

2. **Configuration Section**:
   - Explained the configuration directory structure, emphasizing the use of `{HOME}/.config/dployer`.

3. **Commands Section**:
   - Updated the list of available commands, reflecting the recent changes and functionality.

4. **Development Section**:
   - Added details about the project structure, including the newly introduced `deploy.c` file.