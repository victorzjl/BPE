all:
	gcc -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -march=i686 -c rdkafka.c -o objects/rdkafka.o
	gcc -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -march=i686 -c rdkafka_broker.c -o objects/rdkafka_broker.o
	gcc -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -march=i686 -c rdkafka_msg.c -o objects/rdkafka_msg.o
	gcc -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -march=i686 -c rdkafka_topic.c -o objects/rdkafka_topic.o
	gcc -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -march=i686 -c rdkafka_defaultconf.c -o objects/rdkafka_defaultconf.o
	gcc -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -march=i686 -c rdkafka_timer.c -o objects/rdkafka_timer.o
	gcc -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -march=i686 -c rdkafka_offset.c -o objects/rdkafka_offset.o
	gcc -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -march=i686 -c rdcrc32.c -o objects/rdcrc32.o
	gcc -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -march=i686 -c rdgz.c -o objects/rdgz.o
	gcc -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -march=i686 -c rdaddr.c -o objects/rdaddr.o
	gcc -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -march=i686 -c rdrand.c -o objects/rdrand.o
	gcc -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -march=i686 -c rdthread.c -o objects/rdthread.o
	gcc -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -march=i686 -c rdqueue.c -o objects/rdqueue.o
	gcc -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -march=i686 -c rdlog.c -o objects/rdlog.o
	gcc -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -march=i686 -c snappy.c -o objects/snappy.o
	gcc -shared -Wl,-soname,librdkafka.so.1 -Wl,--version-script=librdkafka.lds  objects/rdkafka.o objects/rdkafka_broker.o objects/rdkafka_msg.o objects/rdkafka_topic.o objects/rdkafka_defaultconf.o objects/rdkafka_timer.o objects/rdkafka_offset.o objects/rdcrc32.o objects/rdgz.o objects/rdaddr.o objects/rdrand.o objects/rdthread.o objects/rdqueue.o objects/rdlog.o objects/snappy.o -o librdkafka.so.1 -lpthread -lz   -lrt
	ar rcs librdkafka.a objects/rdkafka.o objects/rdkafka_broker.o objects/rdkafka_msg.o objects/rdkafka_topic.o objects/rdkafka_defaultconf.o objects/rdkafka_timer.o objects/rdkafka_offset.o objects/rdcrc32.o objects/rdgz.o objects/rdaddr.o objects/rdrand.o objects/rdthread.o objects/rdqueue.o objects/rdlog.o objects/snappy.o

clean:
	rm -f objects/* librdkafka.so.1 librdkafka.a
