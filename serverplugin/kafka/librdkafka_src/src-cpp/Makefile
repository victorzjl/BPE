all:
	g++ -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -Wno-non-virtual-dtor -c RdKafka.cpp -o objects/RdKafka.o
	g++ -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -Wno-non-virtual-dtor -c ConfImpl.cpp -o objects/ConfImpl.o
	g++ -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -Wno-non-virtual-dtor -c HandleImpl.cpp -o objects/HandleImpl.o
	g++ -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -Wno-non-virtual-dtor -c ConsumerImpl.cpp -o objects/ConsumerImpl.o
	g++ -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -Wno-non-virtual-dtor -c ProducerImpl.cpp -o objects/ProducerImpl.o
	g++ -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -Wno-non-virtual-dtor -c TopicImpl.cpp -o objects/TopicImpl.o
	g++ -MD -MP -g -O2 -fPIC -Wall -Wfloat-equal -Wpointer-arith -Wno-non-virtual-dtor -c MessageImpl.cpp -o objects/MessageImpl.o
	gcc  -shared -Wl,-soname,librdkafka++.so.1 objects/RdKafka.o objects/ConfImpl.o objects/HandleImpl.o objects/ConsumerImpl.o objects/ProducerImpl.o objects/TopicImpl.o objects/MessageImpl.o -o librdkafka++.so.1 -lpthread -lz   -lrt -lstdc++
	ar rcs librdkafka++.a objects/RdKafka.o objects/ConfImpl.o objects/HandleImpl.o objects/ConsumerImpl.o objects/ProducerImpl.o objects/TopicImpl.o objects/MessageImpl.o

clean:
	rm -f objects/* librdkafka++.so.1 librdkafka++.a
