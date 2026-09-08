
set OutPutDir=_Output_STM_DCSP_Binary
rmdir /s /q %OutPutDir%
mkdir %OutPutDir%

echo off

set INSTALL_DIR=c:\Program Files (x86)\IAR Systems\Embedded Workbench 6.5\common\bin
set EXEC_NAME=IarBuild.exe
set RUN_EXEC="%INSTALL_DIR%\%EXEC_NAME%"

set BuildAppDir=STM_Application
set BuildBootDir=STM_Bootloader
set ReturnDir=../../

@echo "============> Build STM DCSP Bootloader"
set BuildDir="%BuildBootDir%"
@echo on
cd %BuildDir%
%RUN_EXEC% STM_Bootloader.ewp -build Debug -log all
@echo off
cd ../
copy %BuildDir%\Debug\Exe\*.bin %OutPutDir%

@echo "============> Build STM_Module_PLS.ewp ELS module"
set BuildDir="%BuildAppDir%/ELS"
@echo on
cd %BuildDir%
%RUN_EXEC% STM_Application_ELS.ewp -build Debug -log all
@echo off
cd %ReturnDir%
copy %BuildDir%\Debug\Exe\*.bin %OutPutDir%

@echo "============> Build STM_Module_PLS.ewp PLS module"
set BuildDir="%BuildAppDir%/PLS"
@echo on
cd %BuildDir%
%RUN_EXEC% STM_Application_PLS.ewp -build Debug -log all
@echo off
cd %ReturnDir%
copy %BuildDir%\Debug\Exe\*.bin %OutPutDir%

exit