# Compilation

Compilation requires the following packages:

  * The Qt library. QSoas works only with  Qt5
  * The mruby interpreter, from https://mruby.org/. You need version
    1.4.1, to be downloaded from there:
    https://github.com/mruby/mruby/archive/1.4.1.zip
    Version from 2.0 also work
  * Any Ruby interpreter (you don't need the development files, ruby
    is only used to prepare the build, it is not used to build
    against)
  * The GSL libraries, from http://www.gnu.org/software/gsl/, 
  versions 1.16 until 2.5 are supported

For Debian-based distributions, you should pull all the necessary
dependencies using

~~~
~ apt install ruby-dev libmruby-dev libgsl-dev libqt5opengl5-dev qt5-qmake qtbase5-dev libqt5svg5-dev qttools5-dev
~~~
  

On mac and linux computers, just running

~~~
~ ./configure
~ make
~~~

should do the trick.

If you use a custom build of mruby, not installed in the standard
library search paths, you need to specify it to ./configure

  MRUBY_DIR=/path/to/mruby/dir ./configure

On windows, that should do the trick too, provided the GSL, Qt and
Ruby libraries are in a place where the linker can find it.

Alternatively, you can just use

~~~
~ qmake QSoas.pro MRUBY_DIR=path/to/mruby
~ make
~~~

This produces the `QSoas` executable in place. It is simpler to use it
from there directly.


## Error: missing `mrb_protect`

It can happen that the compilation fails with the error "missing
`mrb_protect`". That comes from missing `mruby-error` mrbgem. Just add
the following line before the `end` in  `mrbgems/default.gembox` in
your mruby source code:

~~~
  conf.gem :core => "mruby-error"
~~~


## Compilation from git

On top of the packages require for the compilation from the tarball,
you'll need some additional tools to generate the various files:

 * for the icons, `inkscape` and `png2icns`
 * for the documentation `dvipng` and `kramdown`, and of course the Qt
   help system
