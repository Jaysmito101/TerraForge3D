python SetupDoxygen.py

vendor\\doxygen\\doxygen

git clone --single-branch --branch gh-pages https://github.com/Jaysmito101/TerraForge3D ./ghpages

robocopy Docs\\html\\ ghpages\\documentation\\ /s /e /MT /IS /IT /IM
