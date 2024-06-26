default:
	gcc lsc.c -o lsc

dbg:
	gcc lsc.c -o lsc -g

.PHONY: clean
clean:
	rm lsc
