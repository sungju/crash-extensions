pstree.so: pstree.c
	    gcc -shared -fPIC -rdynamic -DX86_64 $^ -o $@

clean:
	rm -rf pstree.so
