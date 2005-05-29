@call b\unset.bat
@call b\clean.bat

@set CFI=-Iinclude -I.
@set CFASM=-DLZO_USE_ASM
@set BNAME=lzo
@set BLIB=lzo.lib
@set BDLL=lzo.dll

@echo Compiling, please be patient...
