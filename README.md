**payload_extract**
===========
**Extract the image from payload.bin in Android ROM**

**Supported:**  
- bin
- zip
- url

```
payload_extract --help

usage: [options]
  -h, --help           Display this help and exit
  -i, --input=[PATH]   File path or URL
  --incremental=X      Old directory, Catalog requiring incremental patching
  -p                   Print all info
  -P, --print=X        Print the specified targets: [boot,odm,...]
  -x                   Extract all items
  -X, --extract=X      Extract the specified targets: [boot,odm,...]
  -e                   Exclude mode, exclude specific targets
  -s                   Silent mode, Don't show progress
  -T#                  [1-X] Use # threads, default: -T0, is X/3
  -k                   Skip SSL verification
  -o, --outdir=X       Output dir
  -V, --version        Print the version info
```

```
  payload_extract -V
  
  payload_extract:     v0.0.0-0000000000
  author:              skkk
```

**You can use [extract.erofs](https://github.com/sekaiacg/erofs-utils/releases) to continue extracting data from the erofs format image.**  

**Contributors**
- [affggh](https://github.com/affggh): Windows
