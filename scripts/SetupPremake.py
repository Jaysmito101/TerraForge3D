import sys
import os
from pathlib import Path
import tarfile

import Utils

def GetOSName():
    if sys.platform == 'win32':
        return 'windows'
    elif sys.platform == 'darwin':
        return 'macos'
    elif sys.platform == 'linux':
        return 'linux'
    else:
        return 'unknown'

class PremakeConfiguration:
    premakeVersion = "5.0.0-beta1"
    premakeOS = GetOSName()
    premakeZipURLExtension = "zip" if premakeOS == "windows" else "tar.gz"
    premakeZipUrls = f"https://github.com/premake/premake-core/releases/download/v{premakeVersion}/premake-{premakeVersion}-{premakeOS}.{premakeZipURLExtension}"
    premakeLicenseUrl = "https://raw.githubusercontent.com/premake/premake-core/master/LICENSE.txt"
    premakeDirectory = os.path.join("vendor", "premake")
    premakeFileName = f"premake" + ( ".exe" if sys.platform == 'win32' else "" )

    @classmethod
    def Validate(cls):
        if (not cls.CheckIfPremakeInstalled()):
            print("Premake is not installed.\nInstalling Premake ...")
            if not os.path.exists(cls.premakeDirectory):
                print("Creating Premake directory : " + cls.premakeDirectory)
                os.makedirs(cls.premakeDirectory)
            return  cls.InstallPremake()
            

        print(f"Correct Premake located at {os.path.abspath(cls.premakeDirectory)}")
        return True

    @classmethod
    def CheckIfPremakeInstalled(cls):
        premakeExe = Path(f"{cls.premakeDirectory}/LICENSE.txt")
        if (not premakeExe.exists()):
            return False
        return True

    @classmethod
    def InstallPremake(cls):
        premakePath = f"{cls.premakeDirectory}/premake-{cls.premakeVersion}-{GetOSName()}.{cls.premakeZipURLExtension}"
        print("Downloading {0:s} to {1:s}".format(cls.premakeZipUrls, premakePath))
        Utils.DownloadFile(cls.premakeZipUrls, premakePath)
        print("Extracting", premakePath)
        if(cls.premakeOS == "windows"):
            Utils.UnzipFile(premakePath, deleteZipFile=True)    
        else:
            file = tarfile.open(premakePath)
            file.extractall(cls.premakeDirectory)
            file.close()
            os.remove(premakePath)
        
        print(f"Premake {cls.premakeVersion} has been downloaded to '{cls.premakeDirectory}'")

        premakeLicensePath = f"{cls.premakeDirectory}/LICENSE.txt"
        print("Downloading {0:s} to {1:s}".format(cls.premakeLicenseUrl, premakeLicensePath))
        Utils.DownloadFile(cls.premakeLicenseUrl, premakeLicensePath)
        print(f"Premake License file has been downloaded to '{cls.premakeDirectory}'")

        return True
