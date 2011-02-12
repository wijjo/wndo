#  Copyright (c) 2007 Steve Cooper, steve@wijjo.com
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# (See the included file COPYING / GPL-2.0)

SUBDIRS = bash_completion bin wndo.d debian docgen wndo-ctrl wndo-pick wndo-spy lib test

AUX_DIST = compile config.guess config.log config.sub depcomp \
	install-sh missing py-compile
MAINTAINERCLEANFILES = Makefile.in aclocal.m4 build-stamp configure config.h.in \
	install-stamp stamp-h.in $(AUX_DIST)

GENDOCTEXT = INSTALL README REFERENCE TUTORIAL
GENDOCHTML = install.html readme.html reference.html tutorial.html

noinst_DATA = make-deb make-release AUTHORS BUGS COPYING NEWS TODO $(GENDOCTEXT) $(GENDOCHTML)

EXTRA_DIST  = autogen.sh $(noinst_DATA) new-version

$(GENDOCTEXT) $(GENDOCHTML): dummy

#TODO: There must be a better way to make sure docgen/wndo is availabe when
#      building debian package!  I'm sure there's also a more elegant way to
#      handle documentation generation in general.
dummy:
	test -f docgen/wndo || cp ../docgen/wndo docgen/
	docgen/wndo help install output=INSTALL
	docgen/wndo help readme output=README
	docgen/wndo help reference output=REFERENCE
	docgen/wndo help tutorial output=TUTORIAL
	docgen/wndo help install output=install.html format=html
	docgen/wndo help readme output=readme.html format=html
	docgen/wndo help reference output=reference.html format=html
	docgen/wndo help tutorial output=tutorial.html format=html

# vim: noet
