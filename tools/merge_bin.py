import subprocess

Import("env")

ret = subprocess.run(["git", "describe", "--tags"], stdout=subprocess.PIPE, text=True) # Uses only annotated tags
build_version = ret.stdout.strip()
# print(build_version)

APP_BIN = "$BUILD_DIR/${PROGNAME}.bin"
# MERGED_BIN = "$BUILD_DIR/${PROGNAME}_merged.bin"
# MERGED_BIN = "$BUILD_DIR/ATD3.5-S3-HandySense-PIO." + build_version + ".bin"
MERGED_BIN = "dist/ATD3.5-S3-HandySense-PIO." + build_version + ".bin"
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
                "--chip", BOARD_CONFIG.get("build.mcu", "esp32s3"),
                "merge_bin",
                "-o", MERGED_BIN,
                "--flash_mode", "dio",
                "--flash_freq", "80m",
                "--flash_size", BOARD_CONFIG.get("upload.flash_size", "8MB"),
            ]
            + flash_images
        )
    )

# Add a post action that runs esptoolpy to merge available flash images
env.AddPostAction(APP_BIN, merge_bin)

# Patch the upload command to flash the merged binary at address 0x0
env.Replace(
    UPLOADERFLAGS=[] + [
        "--chip", BOARD_CONFIG.get("build.mcu", "esp32s3"),
        "--port", "\"$UPLOAD_PORT\"",
        "--baud", "$UPLOAD_SPEED",
        "--before", BOARD_CONFIG.get("upload.before_reset", "default_reset"),
        "--after", BOARD_CONFIG.get("upload.after_reset", "hard_reset"),
        "write_flash", "-z",
        "--flash_mode", "${__get_board_flash_mode(__env__)}",
        "--flash_freq", "${__get_board_f_flash(__env__)}",
        "--flash_size", BOARD_CONFIG.get("upload.flash_size", "8MB"),
        "0x0000", MERGED_BIN
    ],
    UPLOADCMD='"$PYTHONEXE" "$UPLOADER" $UPLOADERFLAGS',
)
