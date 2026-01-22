# myshell

A simple Unix shell written in C++.

## Features

- Execute basic commands (ls, cat, echo, etc.)
- Support for pipes (`cmd1 | cmd2 | cmd3`)
- Built-in `quit` command to exit

## Build

```bash
g++ -o myshell myshell.cpp
```

## Run

```bash
./myshell
```

## Usage

```
myshell> ls
myshell> echo hello world
myshell> cat file.txt | grep keyword
myshell> ls -la | grep txt | wc -l
myshell> quit
```

## How It Works

- Uses `fork()` to create child processes
- Uses `execvp()` to run commands
- Uses `pipe()` and `dup2()` for connecting commands with pipes
- Parent waits for children using `waitpid()`

## Requirements

- Linux/Unix environment
- G++ compiler
