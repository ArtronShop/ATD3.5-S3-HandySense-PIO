import subprocess

Import("env")

FILENAME_VERSION_H = "include/version.h"

ret = subprocess.run(["git", "describe", "--tags"], stdout=subprocess.PIPE, text=True) #Uses only annotated tags
build_version = ret.stdout.strip()
print("Firmware Revision: " + build_version)
with open(FILENAME_VERSION_H, 'w') as f:
    f.write(
        "// AUTO CREATE, DO NOT MODIFY\n"
        "#define VERSION \"" + build_version + "\"\n"
    )
