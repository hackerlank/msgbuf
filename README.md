# Message buffer in C++

Given an input file containing message declaration, this simple tool generate the code that allows to easily serialize/unserialize data.

msgbuf tries to implement some features (in fact a very limited subset) supported by tools like [google protobuf] or [ASN.1]

## Characteristics:
* Simple language definition
* Output a **single** standard .h file, without requiring you to link with other libs
* Very **simple** to use
* Uses inheritance (all Messages inherit from a base class)
* Space efficient serialization
* Simple versionning
* Automatically and transparently handle big/little endian conversion
* Portable

msgbuf **doesn't** support (at least) the following features:
* Strong versionning
* Optional fields

**NOTE**: If you have more advanced needs, you might want to look at better (but more complex) alternatives such as [google protobuf] or [ASN.1]

## Compilation:
    $ ./build_gcc.sh    # build on linux/cygwin (static)
or
    $ ./build_mingw.sh  # cross-compile windows exe from linux (static)

## Usage:
(assuming your message definition is in test.mb)

    $ msgbuf test.mb test.h

## TODO
* Enums
* Support message-in-message (?)
* Support variable size array (?)
* Create testsuite/samples
* Better console output about what's happening
* Provide build script
* Test on other platform (at least gcc, mingw and visual studio)

  [google protobuf]: http://code.google.com/p/protobuf/
  [ASN.1]: http://en.wikipedia.org/wiki/Abstract_Syntax_Notation_One
