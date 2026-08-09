target extended-remote :3333
monitor reset halt
load
monitor reset init
break reset_handler
continue
