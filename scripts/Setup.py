
import os
import subprocess
import platform
import sys
import Utils

from SetupPython import PythonConfiguration as PythonRequirements


from SetupPremake import PremakeConfiguration as PremakeRequirements


# Download Libs

def FixLibDep(url, path):
    if not os.path.exists(path):
        print("Downloading " + path + "...")
        Utils.DownloadFile(url, path)

def GenerateProjects():
    # Call Premake to generate projects for windows(vs2022) and linux (gmake2)
    if sys.platform == "win32":
        subprocess.call(["vendor\\premake\\premake5.exe", "vs2022"])
    elif sys.platform == "linux":
        subprocess.call(["./vendor/premake/premake5", "gmake2"])
    else:
        print("Unsupported platform")
        exit(1)
    

def main():
    workingDir = os.getcwd()
    # If working dir is scripts dir, go up one level
    if os.path.basename(workingDir) == "scripts":
        print(workingDir)
        print(os.path.basename(workingDir))
        os.chdir("..")
    
    # Make sure everything we need for the setup is installed
    PythonRequirements.Validate()

    premakeInstalled = PremakeRequirements.Validate()

    if premakeInstalled:
        GenerateProjects()
    else:
        print("Premake not installed")
        exit(1)

if __name__ == "__main__":
    
    main()