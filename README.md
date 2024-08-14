# Dployer

Dployer is a command-line tool designed to manage Git repositories, deploy applications using Docker, and handle SQLite-based configurations.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Usage](#usage)
  - [Initialize a New Repository](#initialize-a-new-repository)
  - [List All Repositories](#list-all-repositories)
  - [Update Repositories](#update-repositories)
  - [Switch Branch or Tag](#switch-branch-or-tag)
  - [Deploy a Repository](#deploy-a-repository)
  - [Clean Up Docker Resources](#clean-up-docker-resources)
- [Building the Project](#building-the-project)
- [Running the Application](#running-the-application)
- [Contributing](#contributing)
- [License](#license)

## Prerequisites

Before you begin, ensure you have the following installed on your system:

- **Git**
- **Docker**
- **SQLite3**
- **JSON-C Library**

On a Linux system, you can install these dependencies using:

```bash
sudo apt-get update
sudo apt-get install build-essential cmake libsqlite3-dev libjson-c-dev git docker.io
```

On macOS, you can install them using Homebrew:

```bash
brew install json-c sqlite3
brew install --cask docker
```

## Installation

1. **Clone the repository**:

    ```bash
    git clone <your-repository-url>
    cd <your-repository-directory>
    ```

2. **Build the project**:

    ```bash
    make
    ```

## Usage

After building the project, you can use `dployer` to manage repositories and deployments. Below are the available commands:

### Initialize a New Repository

To initialize a new repository:

```bash
./dployer new
```

You will be prompted for:

- Repository ID
- Git URL
- Destination folder
- Branch name
- Docker image prefix
- Docker port

### List All Repositories

To list all repositories:

```bash
./dployer list
```

### Update Repositories

To update all repositories:

```bash
./dployer update
```

To update a specific repository by ID:

```bash
./dployer update <ID>
```

### Switch Branch or Tag

To switch a repository to a specific branch or tag:

```bash
./dployer switch <ID> <BRANCH_OR_TAG>
```

### Deploy a Repository

To deploy all repositories:

```bash
./dployer deploy
```

To deploy a specific repository by ID:

```bash
./dployer deploy <ID>
```

### Clean Up Docker Resources

To clean up unused Docker images, networks, and volumes:

```bash
./dployer clean
```

## Building the Project

To build the project, run:

```bash
make
```

This will compile the source files and generate the `dployer` executable.

## Running the Application

After building, you can run the application by executing:

```bash
./dployer <command>
```

Replace `<command>` with one of the commands listed in the [Usage](#usage) section.

## Contributing

Contributions are welcome! Please fork the repository, make your changes, and submit a pull request.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
