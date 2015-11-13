.SILENT:

ECRANX=EcranX
ECRAN=Ecran
GRILLE=Grille

CC=g++ -DSUN -I$(ECRANX) -I$(ECRAN) -I$(GRILLE)
ALL: SpaceInvaders
OBJS = $(ECRAN)/Ecran.o $(ECRANX)/EcranX.o $(GRILLE)/Grille.o  
PROGRAMS = SpaceInvaders

SpaceInvaders:	SpaceInvaders.c $(ECRAN)/Ecran.o $(ECRANX)/EcranX.o $(GRILLE)/Grille.o
	echo Creation de SpaceInvaders ...
	$(CC) SpaceInvaders.c -o SpaceInvaders $(ECRAN)/Ecran.o $(ECRANX)/EcranX.o $(GRILLE)/Grille.o -lrt -L/usr/X11R6/lib -lX11 -lpthread

$(ECRAN)/Ecran.o:	$(ECRAN)/Ecran.c $(ECRAN)/Ecran.h
	echo Creation de Ecran.o ...
	$(CC) -c $(ECRAN)/Ecran.c
	mv Ecran.o $(ECRAN)

$(ECRANX)/EcranX.o:	$(ECRANX)/EcranX.c $(ECRANX)/EcranX.h
	echo Creation de EcranX.o ...
	$(CC) -c $(ECRANX)/EcranX.c
	mv EcranX.o $(ECRANX)

$(GRILLE)/Grille.o:	$(GRILLE)/Grille.c $(GRILLE)/Grille.h
	echo Creation de Grille.o ...
	$(CC) -c $(GRILLE)/Grille.c
	mv Grille.o $(GRILLE)

clean:
	@rm -f $(OBJS) core Trace.log

clobber:	clean
	@rm -f tags $(PROGRAMS)
