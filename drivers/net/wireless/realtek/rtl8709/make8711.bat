;;		
;;
;;				RTL8711 Series Driver Make tool for Windows
;;				
;;				usage:
;;				1. config_xp.exe
;;				2. make8711 driver //for testing
;;				3. make8711 release Main_Ver Minor_Ver



@echo off
if "%1" equ "help" goto PROMPT 
if "%1" equ "xp" goto xp
if "%1" equ "ce" goto ce 
if not exist RTL8711_DRVCFG_XP goto NO_CONFIG
goto  PARAMETER_ERR 


::--------- building driver ---------- 
cls
@echo off
goto BUILD

;;===== build xp driver ===== 
:xp

call core_xp_conf.bat

if "%2" equ "usb" goto usb 
if "%2" equ "sdio" goto sdio
if "%2" equ "cfio" goto cfio  
goto  PARAMETER_ERR 

::===== build ce driver =====
:ce

call core_ce_conf.bat
call set_sources.bat
goto  BUILD

;;===== build sdio driver =====
:sdio
;;cls
@echo off
call intf_sdio.bat
call set_sources.bat
goto BUILD

@echo off
goto BUILD

::===== build usb driver =====
:usb
;;cls
@echo off

call intf_usb.bat
call set_sources.bat


@echo off
goto BUILD

::===== build cfio driver =====
:cfio
;;cls

@echo off

call intf_cfio.bat
call set_sources.bat


@echo off
goto BUILD

;; ---------- NO_CONFIG ---------------
:NO_CONFIG
;cls
@echo on
@echo =======================================================
@echo =======================================================
@echo =========== RTL8711 Easy Driver Config Menu ===========
@echo =======================================================
@echo =======================================================


@echo Can't find RTL8711_DRVCFG_XP in the current dir
@echo off
goto _prompt 

;; ---------- PROMPT ------------------------------------------

:PROMPT
;cls
:_prompt
@echo on
@echo 1. config_xp 
@echo 2. make8711 [xp/ce] [sdio/usb/cfio]
@echo off
goto END


;; ----------- PARAMETER ERR -----------
:PARAMETER_ERR
;cls
@echo on
@echo 1. make8711 [xp/ce] [sdio/usb/cfio]
@echo off
goto END

;; ---------- BUILD ------------------------------------------

:BUILD
DEL Makefile /Q
    build -cZg
if "%1" equ "ce" goto clr_set 
    goto END

;; ---------- clr_set ------------------------------------------
:clr_set

set DEVICE_ID=
set DRIVER_ID=
set DRIVER_NAME=
set mkdriver_target=
set BUILD_OPTIONS=
set mkdriver_driver_path=
	goto END

:END
