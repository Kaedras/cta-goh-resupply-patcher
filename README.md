# Resupply patcher for Call to Arms - Gates of Hell: Ostfront

This is a patcher to quickly update my [Increased Supply](https://steamcommunity.com/sharedfiles/filedetails/?id=3395398956) mod and its variants.

Please note that there has not been any testing on Windows.

## Supported mods

- [Valour](https://steamcommunity.com/sharedfiles/filedetails/?id=2537987794)
- [MACE](https://steamcommunity.com/sharedfiles/filedetails/?id=2905667604)
- [Hotmod 1968](https://steamcommunity.com/sharedfiles/filedetails/?id=2614199156)
- [West 81](https://steamcommunity.com/sharedfiles/filedetails/?id=2897299509)

## Usage

```commandline
resupply_patcher OUTPUT_PATH
```

```commandline
resupply_patcher --valour OUTPUT_PATH
```

A message will be printed if any file inside the output directory has been modified.

## Dependencies

- GCC >= 15.1
- CMake 3.31.7
- libzip 1.11.3
- openssl 3

## Adding support for another mod

Adding support for another mod is pretty straightforward:
1. Add a header file to `src/mods/` by copying e.g. `Mace.h` and modifying the values accordingly.
2. Include it in `src/Mods.h`.
3. Add a command line option in `main()`.
