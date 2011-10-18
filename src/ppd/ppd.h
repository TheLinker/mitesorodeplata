#ifndef PPD_H_
#define PPD_H_
#define TAM_SECT 512
#define TAM_PAG 4096

char comando[15], param[15], buffer[512];
char * pathArch;
void * dirMap, * dirSect;
int sect, cantSect, offset;
div_t res;
size_t len = 100;
FILE * dirArch;

void escucharPedidos(void);

void atenderPedido(void);

void leerPedido(int sect, FILE * dirArch);

void escribirPedido(char param[15], int sect, FILE * dirArch);

FILE * abrirArchivoV(char * pathArch);

void * paginaMap (int sect, FILE * dirArch);

void escucharConsola(void);

void atenderConsola(void);

void funcInfo(void);

void funcClean(void);

void funcTrace(void);




#endif /* PPD_H_ */
