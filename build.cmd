@echo off
echo Building MiniOS kernel...

REM Assemble the initialization code
echo Assembling __init.asm...
utils\nasm.exe -O0 -fwin64 kernel\__init.asm -o kernel\__init.o
if %ERRORLEVEL% NEQ 0 (
    echo Failed to assemble __init.asm
    exit /b 1
)

REM Compile the kernel
echo Compiling kernel...
x86_64-w64-mingw32-gcc -O0 -ffreestanding -nostdlib -Wl,--image-base,0x200000,-e,ASMEntryPoint,--section-alignment,0x1000,--file-alignment,0x1000,--subsystem,native,-T,link_script.ld kernel\__init.o kernel\main.c kernel\logging.c kernel\screen.c kernel\string.c kernel\pic.c kernel\pit.c kernel\rtc.c kernel\keyboard.c kernel\cli.c kernel\irq_dispatch.c kernel\isr_dispatch.cpp kernel\ata.c -o bin\kernel.exe
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile kernel
    exit /b 1
)

REM Create floppy image
echo Creating floppy image...
python utils\makeFloppy.py boot\mbr.asm boot\ssl.asm
if %ERRORLEVEL% NEQ 0 (
    echo Failed to create floppy image
    exit /b 1
)

echo Build complete!
