#!/usr/bin/make -f
# Sample debian/rules that uses debhelper.
# GNU copyright 1997 to 1999 by Joey Hess.

# Uncomment this to turn on verbose mode.
#export DH_VERBOSE=1

# This is the debhelper compatability version to use.
export DH_COMPAT=5
export destdir = $(CURDIR)/debian/gpsdrive/usr

configure: configure-stamp
configure-stamp:
	dh_testdir
	# Add here commands to configure the package.
	cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DMAN_INSTALL_DIR=/usr/share/man -DINFO_INSTALL_DIR=/usr/share/info .

	touch configure-stamp

build: configure-stamp build-stamp
build-stamp:
	dh_testdir

	# Add here commands to compile the package.
	$(MAKE)
	#/usr/bin/docbook-to-man debian/gpsdrive.sgml > gpsdrive.1

	touch build-stamp

clean:
	dh_testdir
	dh_testroot
	rm -f build-stamp configure-stamp

	# Add here commands to clean up after the build process.
	-$(MAKE) distclean

	dh_clean

install: build
	dh_testdir
	dh_testroot
	dh_clean -k
	dh_installdirs

	# Add here commands to install the package into debian/gpsdrive.
	$(MAKE) install prefix=$(destdir)

	# replace non-free script with a wrapper
	# cp $(CURDIR)/debian/gpsfetchmap $(destdir)/bin/
	# chmod +x $(destdir)/bin/gpsfetchmap

	# Remove gpsd, since debian has an own gpsd package
	# rm $(destdir)/bin/gpsd

	# add old manpages
	# cp -r $(CURDIR)/debian/man/ $(destdir)/share/


# Build architecture-independent files here.
binary-indep: build install
# We have nothing to do by default.

# Build architecture-dependent files here.
binary-arch: build install
	dh_testdir
	dh_testroot
#	dh_installdebconf
	dh_installdocs
#	dh_installexamples
#	dh_installmenu
#	dh_installlogrotate
#	dh_installemacsen
#	dh_installpam
#	dh_installmime
#	dh_installinit
#	dh_installcron
	dh_installman
#	dh_installinfo
#	dh_undocumented friendsd2.1 gpsfetchmap.1 wpcvt.1 wpget.1 gpsfetchmap.pl.1 gpspoint2gpsdrive.pl.1 mb2gpsdrive.pl.1 geocache2way.1 gpssql_backup.sh.1 gpssql_restore.sh.1
	dh_installchangelogs ChangeLog
#	dh_link
	dh_strip
	dh_compress
	dh_fixperms
	dh_makeshlibs
	dh_installdeb
#	dh_perl
	dh_shlibdeps
	dh_gencontrol
	dh_md5sums
	dh_builddeb

binary: binary-indep binary-arch
.PHONY: build clean binary-indep binary-arch binary install configure