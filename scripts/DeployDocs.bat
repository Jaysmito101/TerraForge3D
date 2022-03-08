pip install requests

python scripts/SetupDoxygen.py

vendor\\doxygen\\doxygen

cd ..

mkdir TEMP 

(robocopy TerraForge3D\\Docs\\html\\ TEMP\\ /s /e /MT /IS /IT /IM)  ^& exit 0

cd TerraForge3D

git checkout gh-pages

cd ..

(robocopy TEMP\\ TerraForge3D\\documentation\\ /s /e /MT /IS /IT /IM)  ^& exit 0