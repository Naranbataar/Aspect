# Aspect
[![License: Unlicense](https://img.shields.io/badge/license-Unlicense-blue.svg)](http://unlicense.org/)

A simple tool that compares images using image hashing techniques

## Installation
You need just a C compiler with the default libraries, and make

## Example
```shell
$ aspect compute image.jpg image2.jpg
62424443423f3f40444241403f3e3f40  image.jpg
68403f40403f40403f40413f3f404040  image2.jpg
```

```shell
$ aspect compare 62424443423f3f40444241403f3e3f40 68403f40403f40403f40413f3f404040
0.640625  68403f40403f40403f40413f3f404040
```

## Libraries
Using the image loader `stb_image.h` from [stb](https://github.com/nothings/stb)
