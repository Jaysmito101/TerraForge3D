
import os
import subprocess
import platform
import Utils

from SetupPython import PythonConfiguration as PythonRequirements

# Make sure everything we need for the setup is installed
PythonRequirements.Validate()

from SetupPremake import PremakeConfiguration as PremakeRequirements
os.chdir('./../') # Change from devtools/scripts directory to root

premakeInstalled = PremakeRequirements.Validate()

# Download Libs

def FixLibDep(url, path):
    if not os.path.exists(path):
        print("Downloading " + path + "...")
        Utils.DownloadFile(url, path)

print("\nUpdating submodules...")
subprocess.call(["git", "submodule", "update", "--remote", "--recursive", "--merge"])

if (premakeInstalled):
    if platform.system() == "Windows":
        print("\nRunning premake...")
        subprocess.call([os.path.abspath("./scripts/GenProjects.bat"), "nopause"])
    else:
        print("TerraGen3D is only supported on Windows!")

    print("\nSetup completed!")
else:
    print("TerraGen3D requires Premake to generate project files.")
