INCLUDE=-I /usr/lib/mysql
LIB=-L /usr/lib/mysql
LIB_NAME=-lmysqlclient 


.PHONY:all
all:netCal insertData selectData
netCal:netCal.c
	gcc -o $@ $^
insertData:insertData.c
	gcc -o $@ $^ $(INCLUDE) $(LIB) $(LIB_NAME)
selectData:selectData.c
	gcc -o $@ $^ 
.PHONY:clean
clean:
	rm -f netCal insertData selectData

