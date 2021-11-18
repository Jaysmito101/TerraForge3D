
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

# FixLibDep("https://drive.google.com/uc?export=download&id=1kvfpIts1qff2VVKIy3syTkNDZQkQZV4d", "./libs/assimp-vc142-mt.lib")
# FixLibDep("http://download1325.mediafire.com/dqdzails2ctg/8fd3l9cpunlls0a/assimp-vc142-mtd.lib", "./libs/assimp-vc142-mtd.lib")
# FixLibDep("https://drive.google.com/uc?export=download&id=1giYSwEfcNSe7eoN3SGV-8Ud84GBYw1k9", "./libs/IrrXML.lib")
# FixLibDep("https://drive.google.com/uc?export=download&id=13Fv2mTkXA1jbnwo6x-er-jPkHBZRDOE2", "./libs/IrrXMLd.lib")
# FixLibDep("https://drive.google.com/uc?export=download&id=1uyrofXCT0VPKCL3RIu7W0LjUTO51Ewir", "./libs/libcrypto_static.lib")
# FixLibDep("https://drive.google.com/uc?export=download&id=16pQKwMQ2V9vP6tjFMmyLPrHlBtgk6yhp", "./libs/libssl_static.lib")
# FixLibDep("https://drive.google.com/uc?export=download&id=1kqc64aOiNlsAUM4wE7gSN9JtRixHQNgt", "./libs/zlibstatic.lib")
# FixLibDep("https://drive.google.com/uc?export=download&id=1ME_VKN2cHxC-O-i0X9Hf6w3TtSsLBIaq", "./libs/zlibstaticd.lib")

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
