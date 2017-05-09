CFALGS      = -g -O0
INIP_FILES  = tst_iniparser.c ../iniparser.o ../ini.o
TL_FILES    = tst_list.c ../utils.o

all: tst_list tst_iniparser

tst_iniparser: $(INIP_FILES)
	gcc $(INIP_FILES) -o tst_iniparser


tst_list: $(TL_FILES)
	gcc $(TL_FILES) -o tst_list
