import sys
import os
from pathlib import Path

import Utils

class DoxygenConfiguration:
    doxygenZipUrls = f"https://www.doxygen.nl/files/doxygen-1.9.3.windows.x64.bin.zip"
    doxygenLicenseUrl = "https://raw.githubusercontent.com/doxygen/doxygen/master/LICENSE"
    doxygenDirectory = f"./vendor/doxygen"

    @classmethod
    def Validate(cls):
        if (not cls.CheckIfDoxygenInstalled()):
            print("Doxygen is not installed.")
            return False

        print(f"Correct Doxygen located at {os.path.abspath(cls.doxygenDirectory)}")
        return True

    @classmethod
    def CheckIfDoxygenInstalled(cls):
        doxygenExe = Path(f"{cls.doxygenDirectory}/doxygen.exe");
        if (not doxygenExe.exists()):
            return cls.InstallDoxygen()

        return True

    @classmethod
    def InstallDoxygen(cls):
        doxygenPath = f"{cls.doxygenDirectory}/doxygen-windows.zip"
        print("Downloading {0:s} to {1:s}".format(cls.doxygenZipUrls, doxygenPath))
        Utils.DownloadFile(cls.doxygenZipUrls, doxygenPath)
        print("Extracting", doxygenPath)
        Utils.UnzipFile(doxygenPath, deleteZipFile=True)
        print(f"Doxygen has been downloaded to '{cls.doxygenDirectory}'")

        doxygenLicensePath = f"{cls.doxygenDirectory}/LICENSE.txt"
        print("Downloading {0:s} to {1:s}".format(cls.doxygenLicenseUrl, doxygenLicensePath))
        Utils.DownloadFile(cls.doxygenLicenseUrl, doxygenLicensePath)
        print(f"Doxygen License file has been downloaded to '{cls.doxygenDirectory}'")

        return True


DoxygenConfiguration.Validate()