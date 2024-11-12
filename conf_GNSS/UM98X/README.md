# CONFIGURATION UM980 / UM982

#VERSIONA,98,GPS,UNKNOWN,1,161000,0,0,18,139;"UM980","R4.10Build11833","HRPT00-S10C-P","2310415000001-MD22A1241403049","ff3b848f76274f0c","2023/11/24"*9a971f5f

## Push de la config 

### Windows
Installer [UPrecise](https://docs.holybro.com/gps-and-rtk-system/h-rtk-unicore-um982/download)

Via le terminal entrez ligne à ligne la configuration 

### Linux terminal
Se mettre dans le répertoire "rover-gnss/conf_GNSS/UM98X/" du projet
     cat UM982_base.txt > /dev/ttyUSB0 & cat /dev/ttyUSB0
