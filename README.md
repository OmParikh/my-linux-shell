### A brief description of the shell 'myshell'
This shell supports only ls, ps and exit commands.
To compile the file:
```bash
make
```
It will create an executable file named myshell.
Execute the file in this way:
```bash
./myshell
```
Implementation of flags is **not done**, so if you type 
```bash
ls -l
```
 it will **not work**. 
 
 Nevertheless, *ls* can work with any directory. For ex:
```bash
ls /home
```  
will display all files and directories in home directory.

To use *ps* command simply type:
```bash
ps
```

I have used the ***dirent structure*** to implement the ls and ps commands.

For any queries regarding command usage, use the *help* command.
```bash
help
```
Exit the shell using *exit* command:
```bash
exit
```
