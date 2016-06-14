all: dokumentace.pdf

doc.ps: doc.txt
	paps --font="Monospace 10" $< >$@

doc.pdf: doc.ps
	ps2pdf $<

dokumentace.pdf: doc.pdf priloha.pdf
	pdftk $^ cat output $@

clean:
	rm -f doc.ps doc.pdf dokumentace.pdf

remake: clean all

.PHONY: all clean remake
