Staircase switch project using launchpad, mspgcc and interrupts

Author: Ondrej Hejda
Date: 21.6.2012

Description:
Staircase switch (switching light from multiple points) with button input
and led indication.
Output changes when button is pressed.
Leds (or 2color led) are/is smoothly regulated (changing color with diming efect).
Project is used as example application for mspgcc compiler, launchpad and interrupts.
Code::Blocks IDE files are also included (CB's using external Makefile).

Files:
    main.c                      .. source code
    Makefile                    .. makefile (for mspgcc)
    project.cbp, project.layout .. Code::Blocks project files

Revision 17.2.2013: Smaller MCU, door switch and timeout feature.
Project was a bit repurposed to normal light switch used in storeroom. Therefor I've added door switch (for automatic light switching) and automatic switching off timeout. I've also added HW files with pcb designed for the purpose.
