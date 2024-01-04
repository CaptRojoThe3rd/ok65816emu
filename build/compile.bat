
@echo off

C:/MinGW/bin/gcc -Isrc/include -Lsrc/lib -o build/rojo816 src/main.c -lmingw32 -lSDL2main -lSDL2

cd build
::rojo816.exe --bios bios.bin
