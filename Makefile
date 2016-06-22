pstree.so: pstree.c
	    gcc -shared -fPIC -rdynamic -DX86_64 $^ -o $@ -I crash/

clean:
	rm -rf pstree.so pstree.c~
