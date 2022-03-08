pip install requests

python scripts/SetupDoxygen.py

vendor\\doxygen\\doxygen

git clone --single-branch --branch gh-pages https://github.com/Jaysmito101/TerraForge3D ./ghpages

cd ..

mkdir TEMP 

(robocopy TerraForge3D\\Docs\\html\\ TEMP\\ /s /e /MT /IS /IT /IM)  ^& exit 0

cd TerraForge3D

git checkout gh-pages

cd ..

(robocopy TEMP\\ TerraForge3D\\documentation\\ /s /e /MT /IS /IT /IM)  ^& exit 0