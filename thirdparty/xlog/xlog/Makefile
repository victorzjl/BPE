all:
	rm -rf ./lib/libxlog.a
	rm -rf ./lib/libxlog.so
	g++ -fPIC *.cpp -g -O2 -c -I ../log4cplus-1.0.3/include
	cp ../log4cplus-1.0.3/src/.libs/*.o ./
	ar rvs ./lib/libxlog.a *.o
	ranlib ./lib/libxlog.a
	gcc -fPIC -o ./lib/libxlog.so -shared *.o ../log4cplus-1.0.3/src/.libs/liblog4cplus.a -Wall
	rm -rf *.o
clean:
	rm -rf ./lib/libxlog.a
	rm -rf ./lib/libxlog.so
