<!--
SPDX-FileCopyrightText: 2026 Maarten L. Hekkelman

SPDX-License-Identifier: BSD-2-Clause
-->

zeem
====

This library is a C++ library implementing a validating XML parser, a DOM tree, XPaths and serialization. It can be built as either a Module library or a traditional library.

Full documentation is available [here](https://www.hekkelman.net/docs/libzeem/)

> **NOTE** This library was initially named mxml, but that name was already taken.

Building
--------

In order to build this software you need very recent versions of CMake (at least version 3.28) and compilers, most likely at least version 22 of CLang or version 15 of gcc.

```bash
git clone https://forge.hekkelman.net/maarten/zeem.git
cd zeem
cmake -B build -G Ninja
cmake --build build
cmake --install build
```

 If you want to build the module library, you will need at least gcc 15 or a recent clang. And then you will have to add a parameter to cmake, like this:

 ```bash
git clone https://forge.hekkelman.net/maarten/zeem.git
cd zeem
cmake -B build -G Ninja -DZEEM_BUILD_CXX_MODULE=ON
cmake --build build
cmake --install build
```

