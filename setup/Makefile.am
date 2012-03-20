.setupdir:
	$(RM) -rf setupdir
	$(MAKE) -C $(top_builddir) DESTDIR=$(abs_builddir)/setupdir \
		transform= prefix=/ exec_prefix=/ libdir=/lib bindir=/bin \
		datadir=/share includedir=/include install
	cp -av $(top_builddir)/doc/html setupdir/doc
	touch $@

.dlldeps: .setupdir findrequires.pl
	env DEPDEBUG=1 perl findrequires.pl "$(DLLPATH)" setupdir/bin/*.{exe,dll} 2>$@|xargs cp -t setupdir/bin
	touch $@

.stripsetup: .setupdir
	$(STRIP) --strip-unneeded setupdir/bin/*.exe `ls setupdir/bin/*.dll|grep -v cyg`
	touch $@

nativesetup: .setupdir .dlldeps .stripsetup
	"$(ISCC)" "$(srcdir)/ehs.iss"

crosssetup: .setupdir .dlldeps .stripsetup isccwrap.sh
	sh isccwrap.sh "$(srcdir)/ehs.iss"

setup: $(SETUP)

EXTRA_DIST = ehs.iss version.iss
MAINTAINERCLEANFILES = Makefile.in
CLEANFILES = -r .setupdir .dlldeps .stripsetup setupdir