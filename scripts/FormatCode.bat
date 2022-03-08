@echo off
cd ..
astyle  --style=allman --indent=tab --indent-switches --break-blocks=all --pad-comma --delete-empty-lines --align-pointer=name --align-reference=name --break-closing-braces --break-one-line-headers --add-braces   --recursive  ./TerraForge3D/src/*.cpp,*.h,*.hpp