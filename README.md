**payload_extract**
===========
**Extract the image from payload.bin in Android ROM**

**Supports _full_ and _incremental_ payload.bin**

**Supported:**

- bin
- zip
- url

**Help:**

```console
$ payload_extract --help
usage: [options]
  -h, --help           Display this help and exit
  -i, --input=[PATH]   File path or URL
  --incremental=X      Old directory, Catalog requiring incremental patching
  --verify-update        In the incremental mode, The dm-verify verified file
                         does not contain HASH_TREE and FEC. Only files that
                         have successfully updated this information can undergo
                         SHA256 verification.
  --verify-update=X      Only Verify and update the specified targets: [boot,odm,...]
  -p                   Print all info
  -P, --print=X        Print the specified targets: [boot,odm,...]
  -x                   Extract all items
  -X, --extract=X      Extract the specified targets: [boot,odm,...]
  -e                   Exclude mode, exclude specific targets
  -s                   Silent mode, Don't show progress
  -T#                  [1-X] Use # threads, default: -T0, is 1/3
  -k                   Skip SSL verification
  -o, --outdir=X       Output dir
  -V, --version        Print the version info
```

```console
$ payload_extract -V
  payload_extract:     v0.0.0-0000000000
  author:              skkk
```

**_Example:_**

- Extract the full payload.bin(zip/url)

```console
$ ./payload_extract -i payload.bin -o ./full -x
```

- Extract the specified image from the full payload.bin

```console
$ ./payload_extract -i payload.bin -o ./full -X boot,odm,system
```

- Extract all images files except **boot** from the full payload.bin

```console
$ ./payload_extract -i payload.bin -o ./full -X boot -e
```

- Extract all images(Perform verify-update) from incremental payload.bin

The `full` directory contains a complete extraction and verification(verify-update) of the previous payload.bin
```console
$ ./payload_extract -i payload.bin  --incremental ./full -o ./full_patched -x --verify-update
```

- Extract all images(Do not perform verify-update) from incremental payload.bin

```console
$ ./payload_extract -i payload.bin  --incremental ./full -o ./full_patched -x
```

- Extract the **boot** and **odm** images(Do not perform verify-update) from incremental payload.bin
  and calculate the SHA256.

```console
$ ./payload_extract -i payload.bin  --incremental ./full -o ./full_patched -X boot,odm

$ sha256sum ./full_patched/boot.img
d19c097ea9240712a4652df86f3911a721242f275dd32c5ab5b474f5d01528a8  ./full_patched/boot.img
```

- Output the SHA256 of the **boot** image from the incremental payload.bin

```console
$ ./payload_extract -i payload.bin -P boot
PartitionSize:  55 MinorVersion:  9 SecurityPatchLevel: 2025-10-01
name: boot               size: 100663296    sha256: 73fc2ce02d6b6b3f4bef6419b99e09d1e5ea690edaa0b80adced20f13730f3f6
```

- _**verify-update**_ the **boot** and **odm** images from incremental payload.bin
  and calculate the **SHA256**

```console
$ ./payload_extract -i payload.bin --incremental ./full -o ./full_patched --verify-update=boot,odm

$ sha256sum ./full_patched/boot
73fc2ce02d6b6b3f4bef6419b99e09d1e5ea690edaa0b80adced20f13730f3f6  ./full_patched/boot.img
```

**You can use [extract.erofs](https://github.com/sekaiacg/erofs-utils/releases) to continue extracting data from the erofs format image.**

**Contributors**

- [affggh](https://github.com/affggh): Windows
