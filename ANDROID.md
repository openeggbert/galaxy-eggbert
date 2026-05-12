# Android Build Instructions — galaxy-eggbert

This document explains how to build, install, and run galaxy-eggbert as an
Android APK using SDL3, the Android NDK, and CMake.

---

## Prerequisites

| Tool | Recommended version |
|------|---------------------|
| Android Studio | Ladybug (2024.2) or newer |
| Android SDK | API level 35 |
| Android NDK | 28.2.13676358 (installed via SDK Manager) |
| CMake (NDK bundle) | 3.21+ (installed via SDK Manager) |
| Java (JDK) | 17 (bundled with Android Studio) |
| Git | any recent version |

### Install NDK and CMake via Android Studio

1. Open **Android Studio → Settings → SDK Manager → SDK Tools**.
2. Check **NDK (Side by side)** version **28.2.13676358**.
3. Check **CMake** (version 3.21 or higher).
4. Click **Apply** and let Android Studio download and install.

---

## Clone and initialise submodules

```bash
git clone <repository-url> speedy-blupi-2013
cd speedy-blupi-2013
git submodule update --init --recursive
```

The vendored SDL3 / SDL_image / SDL_mixer sources are required.  They live under
`galaxy-eggbert/../cna/third_party/`.

---

## Build the debug APK

```bash
cd galaxy-eggbert/android
./gradlew assembleDebug
```

The first build downloads Gradle (≈ 150 MB) and compiles the NDK libraries, so
it may take several minutes.  Subsequent incremental builds are much faster.

The produced APK is at:

```
galaxy-eggbert/android/app/build/outputs/apk/debug/app-debug.apk
```

---

## Install and run on a device or emulator

Enable **USB debugging** on your device (or start an AVD in Android Studio).

```bash
# Install
adb install app/build/outputs/apk/debug/app-debug.apk

# Launch
adb shell am start -n org.openeggbert.speedyblupi/.SpeedyBlupiActivity

# View logs (filter by the app's tag or by pid)
adb logcat -s SDL SpeedyBlupi
```

To uninstall:

```bash
adb uninstall org.openeggbert.speedyblupi
```

---

## Creating release signing key

Run this once to generate a keystore.  Keep the keystore and passwords safe — you
will need them for every future update.

```bash
cd android
keytool -genkeypair -v \
  -keystore speedy-blupi-release.keystore \
  -alias speedy-blupi \
  -keyalg RSA \
  -keysize 2048 \
  -validity 10000
```

> ⚠️ **Do not commit the keystore to version control.**
> ⚠️ **Do not lose the keystore.** If it is lost, you cannot publish future updates
> signed with the same key — users will have to uninstall and reinstall the app.

---

## Creating key.properties

Create the file `android/key.properties` (one directory above `app/`) with your
real passwords:

```properties
storeFile=speedy-blupi-release.keystore
storePassword=YOUR_STORE_PASSWORD
keyAlias=speedy-blupi
keyPassword=YOUR_KEY_PASSWORD
```

> ⚠️ **Do not commit `key.properties` to version control.**
> Both `key.properties` and `*.keystore` are listed in `.gitignore`.

---

## Building signed release APK

```bash
cd android
export JAVA_HOME=/home/robertvokac/Downloads/openjdk-17.0.2_linux-x64_bin/jdk-17.0.2
./gradlew clean assembleRelease
```

Release APK output:

```
android/app/build/outputs/apk/release/app-release.apk
```

If `key.properties` is not present, the build will stop immediately with a clear
error message rather than a confusing Gradle failure.

---

## Verifying APK signature

```bash
apksigner verify --verbose app/build/outputs/apk/release/app-release.apk
```

---

## Installing release APK on a phone

```bash
adb install -r app/build/outputs/apk/release/app-release.apk
```

> Users who install the APK from outside the Play Store may need to enable
> **Install unknown apps** in their Android settings (Settings → Apps → Special
> app access → Install unknown apps).

---

## Supported ABIs

The current Gradle configuration builds for **arm64-v8a** only.  To add other
ABIs (e.g. `x86_64` for the emulator) edit
`android/app/build.gradle` and extend the `abiFilters` list:

```groovy
abiFilters 'arm64-v8a', 'x86_64'
```

---

## Asset layout

Game assets are packaged into the APK as Android assets.  At runtime they are
read via `SDL_IOFromFile` which transparently falls back to the APK
`AAssetManager` when a file is not found in internal storage.

| Source directory | Path in APK |
|------------------|-------------|
| `Content/backgrounds/` | `Content/backgrounds/` |
| `Content/icons/`       | `Content/icons/` |
| `Content/sounds/`      | `Content/sounds/` |
| `worlds/`              | `worlds/` |

---

## Writable / persistent storage

Save data and configuration are written to the app's private internal storage
via `SDL_GetPrefPath("org.openeggbert", "speedyblupi")`.  This storage:

- persists across app restarts,
- is cleared when the app is uninstalled,
- is **not** accessible to other apps.

---

## Android launcher icon

The launcher icon is generated from `icon.bmp` into `res/mipmap-*` PNG files.
Android does not use BMP directly for launcher icons.
If `icon.bmp` changes, regenerate the PNG density variants with:

```bash
# Flat icons
convert icon.bmp -resize 48x48   android/app/src/main/res/mipmap-mdpi/ic_launcher.png
convert icon.bmp -resize 72x72   android/app/src/main/res/mipmap-hdpi/ic_launcher.png
convert icon.bmp -resize 96x96   android/app/src/main/res/mipmap-xhdpi/ic_launcher.png
convert icon.bmp -resize 144x144 android/app/src/main/res/mipmap-xxhdpi/ic_launcher.png
convert icon.bmp -resize 192x192 android/app/src/main/res/mipmap-xxxhdpi/ic_launcher.png

# Round icons (circular crop)
for density_size in "mdpi:48" "hdpi:72" "xhdpi:96" "xxhdpi:144" "xxxhdpi:192"; do \
  density="${density_size%%:*}"; size="${density_size##*:}"; half=$((size/2)); \
  convert icon.bmp -resize ${size}x${size} \
    \( +clone -alpha extract -draw "fill white circle ${half},${half} ${half},0" \) \
    -alpha off -compose CopyOpacity -composite \
    android/app/src/main/res/mipmap-${density}/ic_launcher_round.png; \
done
```

---

## Troubleshooting

### Gradle build fails: "NDK not found"

Make sure the NDK version in `android/app/build.gradle` (`ndkVersion`)
matches what you installed in the SDK Manager.

### `adb: device not found`

- Enable **Developer Options** and **USB debugging** on your Android device.
- Try `adb devices` to confirm the device is listed.

### App crashes on launch

Check `adb logcat` for native crash details.  Common causes:

- Missing `libmain.so` — rebuild with `./gradlew assembleDebug`.
- Asset not found — verify `git submodule update --init --recursive` completed
  successfully and `Content/` exists under `galaxy-eggbert/`.

### Audio is silent

SDL_mixer must be built with OGG/WAV support.  The vendored build enables WAV
by default.  If you see mixer errors in logcat, check that the sound files are
present in the APK using:

```bash
aapt dump resources app/build/outputs/apk/debug/app-debug.apk | grep sound
```
