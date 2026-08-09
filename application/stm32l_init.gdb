target extended-remote :3333
monitor reset halt
load
monitor reset init
set pagination off
break main_app
layout src
focus cmd
continue
