## Shell Lab

**make sure to move all the trace files from traces/ to current directory**

## function description that i implemented

`eval()`: calls `parseline()` to parse the line and forks a process, apply signal masking for process synchronistaion, calls waitfg.

`builtin_command`: if the command entered is a builtin command execute it first.

`do_bgfg()`: execute the `bg` and `fg` command. sends `SIGCONT` to the process.

`waitfg()`: waits for the foreground process to finish by calling `sigsuspend()`.

`sigchld_handler()`: reaps all the child process and prints a message on the screen.

`sigint_handler()`: sends `SIGINT` signal to all the process in the foreground process group.

`sigtstp_handler()`: send `SIGTSTP` signal to all the process in the poreground process froup.


```

################
CS:APP Shell Lab
################

Files:

Makefile	# Compiles your shell program and runs the tests
README		# This file
tsh.c		# The shell program that you will write and hand in
tshref		# The reference shell binary.

# The remaining files are used to test your shell
sdriver.pl	# The trace-driven shell driver
trace*.txt	# The 15 trace files that control the shell driver
tshref.out 	# Example output of the reference shell on all 15 traces

# Little C programs that are called by the trace files
myspin.c	# Takes argument <n> and spins for <n> seconds
mysplit.c	# Forks a child that spins for <n> seconds
mystop.c        # Spins for <n> seconds and sends SIGTSTP to itself
myint.c         # Spins for <n> seconds and sends SIGINT to itself

```
