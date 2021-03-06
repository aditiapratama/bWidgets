bWidgets
========

bWidgets is a cross-platform widget toolkit, designed specifically for
[Blender](https://www.blender.org/), but reusable as a standalone library.

Further documentation:
* [Documentation landing page](https://julianeisel.github.io/bWidgets/index.html)
  (subpages contain source-code documentation).
* [High level code design documentation](https://julianeisel.github.io/bWidgets/md_docs_bWidgets_about.html).
* [Build instructions](https://julianeisel.github.io/bWidgets/md_docs_build_instructions.html).


# Supported Platforms

bWidgets itself only requires a compiler supporting the C++11 ABI.<br/>
The demo application however requires Freetype2 and OpenGL 3.3 to compile/run.

bWidgets and its demo application have successfully been built on following
platforms:
* Linux Debian 9, GCC 6.3
* Windows 7, MSVC 2017 (only compiled, couldn't launch due to OpenGL driver
issues)
* Windows 10, MSVC 2017

More platforms/compilers should be tested.
