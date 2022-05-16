# UNIX Shell

This bash like shell was built for the partial fulfillment of the course IS F462 - Networking Programming

## Objectives

### UNIX commands
* Shell waits for the user to enter a command. User can enter a command with multiple arguments 
* ![Normal UNIX Commands](Media/1.png)


### Redirection operators
* Shell supports >, < and >> redirection operators. 
* ![Redirection operators](Media/2.png)

### Single pipelined commands
* Shell supports any number of commands in the pipeline.
* ![Single pipe](Media/3.png)

### Double pipeline operator ' || '
* Shell supports "||" (double pipe operator). Ex -: ls -l || grep ^-, grep ^d. It means that output of ls -l command is passed as input to two other commands. 
* ![Double Pipe](Media/4.png)

### Triple pipeline operator ' ||| '
* Shell supports "|||" (triple pipe operator). Ex -: ls -l || grep ^-, grep ^d, grep ^q. It means that output of ls -l command is passed as input to three other commands. 
* ![Double Pipe](Media/5.png)

### Short cut commands mode
* Shell supports sc mode executed by command sc.  In this mode, a command can be executed by pressing Ctrl-C and pressing a number. This number corresponds to the index in the look up table created and deleted by the commands **sc -i <index> <cmd>** / **sc -d <index>**.
  * Inserting a command at particular index in the lookup table.
  * ![Insert in lookup](Media/6.png)
  * Deleting a command from a particular index. 
  * ![Delete from lookup](Media/7.png)
  * Printing the lookup table. 
  * ![Print lookup](Media/8.png)
  * Executing a command from lookup table 
  * ![Execute](Media/9.png)
