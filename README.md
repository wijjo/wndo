# WnDO

## Description

WnDO is a small suite of complementary tools for adding behavioral extensions to Gnome , Xfce , Fluxbox , and other window managers. WnDO offers a plugin platform for tweaking the desktop environment. It focuses on modifying behavior, rather than adding desktop widgets. It has minimal dependencies, other than Python.

Included modules handle window alignment, simple window tiling, process management, wallpaper management, and custom keyboard shortcuts. Wallpaper management capabilities include per-workspace and randomly-selected wallpaper. 

It performs its own key grabbing in order to avoid interaction and conflict with the equivalent window manager facility.

Features are packaged as modules containing exported functions. These functions can be used as commands from the command line. The commands also work well as key or menu bindings. The following example waits for you to click on a window and then moves and resizes the selected window to fill the left side of the screen. See the Tutorial for more examples.

    wndo window.tile 1 3 w=pick

Most features work with any window manager that is fully or mostly compatible with EWMH standards . Automatic user setup works with Gnome, Xfce and Fluxbox. It will also work for other window managers having similar configuration structure.

WnDO is built on top of CmDO (pronounced "commando"), a Python facility that converts a set of "decorated" functions to a complete command-driven environment, with full help. See the required CmDO package for more information.

New features can be installed simply by dropping module files into a user or shared directory.
