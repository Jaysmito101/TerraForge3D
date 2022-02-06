@echo off
cd ..
echo Setting Up Developer Environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
SET Platform=
echo Building
msbuild /m /p:PlatformTarget=x64 /p:Configuration=Debug TerraForge3D.sln
pause
echo on
cls