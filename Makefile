all: shared_mem \
	 reader \
	 writer

shared_mem: shared_mem.o reader_writer_Implementation.o
	gcc shared_mem.o reader_writer_Implementation.o -o shared_mem -lpthread

shared_mem.o: shared_mem.c
	gcc -c shared_mem.c -o shared_mem.o

reader: reader.o reader_writer_Implementation.o
	gcc reader.o reader_writer_Implementation.o -o reader -lpthread

reader.o: reader.c
	gcc -c reader.c -o reader.o

writer: writer.o reader_writer_Implementation.o
	gcc writer.o reader_writer_Implementation.o -o writer -lpthread

writer.o: writer.c
	gcc -c writer.c -o writer.o

reader_writer_Implementation.o: reader_writer_Implementation.c
	gcc -c reader_writer_Implementation.c -o reader_writer_Implementation.o

clean:
	rm -f *.o shared_mem
	rm -f *.o reader
	rm -f *.o writer