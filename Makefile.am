ACLOCAL_AMFLAGS = -I ./aclocal

SUBDIRS = src test examples

dist-hook:
	find $(distdir) -type f \( -iname ".deps" -or -iname ".libs" -or -iname ".gitignore" \
		-or -iname ".DS_Store" -or -iname "._*" \) | xargs rm -rf
	find $(distdir) -type d \( -iname ".svn" -or -iname ".git" \) | xargs rm -rf

print-version:
	@echo $(VERSION)

EXTRA_DIST = LICENSE ChangeLog
CLEANFILES=core* tag cscope.* *.log .*.sw* *~
DISTCLEANFILES=$(CLEANFILES)
