pip install requests

python scripts/SetupDoxygen.py

vendor\\doxygen\\doxygen

git clone --single-branch --branch gh-pages https://github.com/Jaysmito101/TerraForge3D ./ghpages

mkdir ghpages\\documentation > nul

xcopy Docs\\html ghpages\\documentation\\ /E /T /H /C /R /Q /Y