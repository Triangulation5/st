# Triangulation's custom fork of Luke Smith's build of st (the simple (suckless) terminal)

The [suckless terminal (st)](https://st.suckless.org/) with some additional
features that make it literally the best terminal emulator ever:

## Unique features

+ **follow urls** by pressing `alt-l`
+ **copy urls** in the same way with `alt-y`
+ **copy the output of commands** with `alt-o`

## Bindings for

+ **scrollback** with `alt-↑/↓` or `alt-pageup/down` or `shift` while scrolling the
  mouse.
+ OR **vim-bindings**: scroll up/down in history with `alt-k` and `alt-j`.
  Faster with `alt-u`/`alt-d`.
+ **zoom/change font size**: same bindings as above, but holding down shift as
  well. `alt-home` returns to default
+ **copy text** with `alt-c`, **paste** is `alt-v` or `shift-insert`

## Pretty stuff

+ Kitty-matched ANSI colors, hardcoded in `config.h`.
+ Transparency/alpha, which is also adjustable from your `Xresources`.
+ Default font is JetBrainsMono Nerd Font Mono at 14pt.

## Other st patches

+ Boxdraw
+ Ligatures
+ font2
+ updated to latest version 0.8.5

## Installation for newbs

You should have xlib header files and libharfbuzz build files installed.

```
git clone https://github.com/Triangulation5/st
cd st
sudo make install
```

Obviously, `make` is required to build. `fontconfig` is required for the
default build, since it asks `fontconfig` for your system monospace font. It
might be obvious, but `libX11` and `libXft` are required as well. Chances are,
you have all of this installed already.

On OpenBSD, be sure to edit `config.mk` first and remove `-lrt` from the
`$LIBS` before compiling.

Be sure to have a composite manager (`xcompmgr`, `picom`, etc.) running if you
want transparency.

## How to configure dynamically with Xresources

Font and opacity settings can be read from `~/.Xdefaults` or `~/.Xresources`.
Run `xrdb` on one of these files to load the settings. The terminal palette is
hardcoded in `config.h` and is not overridden by Xresources or pywal.

For example, you can define your desired font and transparency:

```
*.font:	Liberation Mono:pixelsize=12:antialias=true:autohint=true;
*.alpha: 0.9
```

The `alpha` value (for transparency) goes from `0` (transparent) to `1`
(opaque). There is an example `Xdefaults` file in this respository.

### Colors

To be clear about the color settings:

- The Kitty palette is hardcoded in `config.h`.
- Xresources and pywal do not override the terminal colors.

Opacity remains configurable through Xresources.

## Credits

[Luke Smith](https://github.com/lukesmithxyz)
