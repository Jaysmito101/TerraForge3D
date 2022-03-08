pip install requests

python scripts/SetupDoxygen.py

vendor\\doxygen\\doxygen

git clone --single-branch --branch gh-pages https://github.com/Jaysmito101/TerraForge3D ./ghpages

del /f /s /q ghpages\\documentation 1>nul

rmdir /s /q ghpages\\documentation

xcopy Docs\\html ghpages\\documentation\\ /E /T /H /C /R /Q /Y