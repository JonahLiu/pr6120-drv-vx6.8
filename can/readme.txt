To use this driver, place following files into given directories and replace 
the original files.

02wnCAN.cdf                     installDir/vxworks-6.x/target/config/comps/VxWorks
03wnCAN_PR6120_CAN.cdf          installDir/vxworks-6.x/target/config/comps/VxWorks
canBoard.h                      installDir/vxworks-6.x/target/h/CAN
pr6120_can.c                    installDir/vxworks-6.x/target/src/drv/CAN
pr6120_can.h                    installDir/vxworks-6.x/target/h/CAN/private
pr6120_can_cfg.c                installDir/vxworks-6.x/target/config/comps/src/CAN
sys_pr6120_can.c                installDir/vxworks-6.x/target/config/comps/src/CAN