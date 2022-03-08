@echo off
cd ..
echo Setting Up Developer Environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
SET Platform=
echo Building Debug
msbuild /m /p:PlatformTarget=x64 /p:Configuration=Debug TerraForge3D.sln
pause
echo on
cls