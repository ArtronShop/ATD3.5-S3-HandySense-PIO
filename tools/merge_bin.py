import subprocess

Import("env")

ret = subprocess.run(["git", "describe", "--tags"], stdout=subprocess.PIPE, text=True) # Uses only annotated tags
build_version = ret.stdout.strip()
# print(build_version)

APP_BIN = "$BUILD_DIR/${PROGNAME}.bin"
# MERGED_BIN = "$BUILD_DIR/${PROGNAME}_merged.bin"
MERGED_BIN = "$BUILD_DIR/ATD3.5-S3-HandySense-FW.{}.bin".format(build_version)
BOARD_CONFIG = env.BoardConfig()

# print(MERGED_BIN)

def merge_bin(source, target, env):
    # The list contains all extra images (bootloader, partitions, eboot) and
    # the final application binary
    flash_images = env.Flatten(env.get("FLASH_EXTRA_IMAGES", [])) + ["$ESP32_APP_OFFSET", APP_BIN]

    # Run esptool to merge images into a single binary
    env.Execute(
        " ".join(
            [
                "$PYTHONEXE",
                "$OBJCOPY",
                "--chip",
                BOARD_CONFIG.get("build.mcu", "esp32s3"),
                "merge_bin",
                "--fill-flash-size",
                BOARD_CONFIG.get("upload.flash_size", "8MB"),
                "-o",
                MERGED_BIN,
            ]
            + flash_images
        )
    )

# Add a post action that runs esptoolpy to merge available flash images
env.AddPostAction(APP_BIN , merge_bin)

# Patch the upload command to flash the merged binary at address 0x0
"""
env.Replace(
    UPLOADERFLAGS=[
        ]
        + ["0x0000", MERGED_BIN],
    UPLOADCMD='"$PYTHONEXE" "$UPLOADER" $UPLOADERFLAGS',
)
"""
