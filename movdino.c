#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif
//Начальные параметры для работы программы
int display=1;
int save=1;
int interval=1.0;
int kstr=0;
//структура для сохранения предыдущего состояния поля
typedef struct {
    char **field;
    char **paint;
    int x, y;
} State;
static State undo_list[10];
static int current_state = -1;
static int total_states = 0;
//функция очистки терминала
void clear(){
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}
//функция задержки между показами поля
void delay(){
#ifdef _WIN32
    Sleep((int)(interval * 1000));
#else
    sleep((int)interval);
#endif
}
//функция копирования поля в функции save state
char** copy_array(char **src, int rows, int cols) {
    char **copy = (char**)malloc(rows * sizeof(char*));
    for (int i = 0; i < rows; i++) {
        copy[i] = (char*)malloc(cols * sizeof(char));
        memcpy(copy[i], src[i], cols);
    }
    return copy;
}
//функция сохранения состояния поля
void save_state(char **field, char **paint, int y, int x, int field_y, int field_x) {
    if (total_states >= 100) {
        fprintf(stderr, "Warning: undo history full\n");
        return;
    }
    int idx = total_states;
    undo_list[idx].field = copy_array(field, field_y, field_x);
    undo_list[idx].paint = copy_array(paint, field_y, field_x);
    undo_list[idx].x = x;
    undo_list[idx].y = y;
    current_state = idx;
    total_states++;
}
//функция для избежания переполнения массива историй состояний поля
void free_state(State *state, int rows) {
    if (state->field) {
        for (int i = 0; i < rows; i++) free(state->field[i]);
        free(state->field);
        state->field = NULL;
    }
    if (state->paint) {
        for (int i = 0; i < rows; i++) free(state->paint[i]);
        free(state->paint);
        state->paint = NULL;
    }
}
//функция для загрузки поля из истории
void load_state(int index, char ***field, char ***paint, int *y, int *x, int field_y, int field_x) {
    if (index < 0 || index >= total_states) {
        fprintf(stderr, "Cannot load state %d (total states: %d)\n", index, total_states);
        return;
    }
    for (int i = 0; i < field_y; i++) {
        memcpy((*field)[i], undo_list[index].field[i], field_x);
        memcpy((*paint)[i], undo_list[index].paint[i], field_x);
    }
    *y = undo_list[index].y;
    *x = undo_list[index].x;
}
//функция для аварийного выхода из прграммы, очищает динамическую память и закрывает файл
void err(FILE *f, char **arr, int h,char **ap) {
    for (int i = 0; i < total_states; i++) {
        free_state(&undo_list[i], h);
    }
    if (f) fclose(f);
    if (arr && h>0) {
        for(int i=0;i<h;i++){
            if (arr[i]) free(arr[i]);
        }
        free(arr);
    }
    if (ap && h>0) {
        for(int i=0;i<h;i++){
            if (ap[i]) free(ap[i]);
        }
        free(ap);
    }
    exit(1);
}
//функиця печатания текущего состояния поля
void print(char **a, int y, int x, int y0, int x0,char **ap) {
    if (display==0) return;
    else {for (int i=0;i<y;i++) {
        for (int j=0;j<x;j++) {
            if (ap[i][j]=='_'){
                if (i==y0 && j==x0) printf("#");
                else printf("%c",a[i][j]);
            }
            else printf("%c",ap[i][j]);
        }
        printf("\n");
    }
    fflush(stdout);
}
}
//функия для пересчета координат, вышедших за границы пол
void check(int *ych,int *xch,int yr,int xr){
    if (*xch>=xr) *xch=0;
    if (*ych>=yr) *ych=0;
    if (*xch<0) *xch=xr-1;
    if (*ych<0) *ych=yr-1;
}
//функия для прыжка
void jump(int kj,int *ym,int *xm,char ***aa,int yr,int xr,char napr,FILE *f,char ***ap){
            if ((*aa)[*ym][*xm]=='#') (*aa)[*ym][*xm]='_';
            for (int i=1;i<=kj;i++){
            if (napr=='U') *ym=*ym-1;
            else if (napr=='D') *ym=*ym+1;
            else if (napr=='L') *xm=*xm-1;
            else if (napr=='R') *xm=*xm+1;
            check(ym,xm,yr,xr);
            if((*aa)[*ym][*xm]=='^' || (*aa)[*ym][*xm]=='&') {
                if (napr=='U') *ym=*ym+1;
                else if (napr=='D') *ym=*ym-1;
                else if (napr=='L') *xm=*xm+1;
                else if (napr=='R') *xm=*xm-1;
                check(ym, xm, yr, xr);
                (*aa)[*ym][*xm]='#';
                fprintf(stderr, "Warning in %d line: failed to JUMP\n",kstr);
                break;
            }
            }
            if ((*aa)[*ym][*xm]=='%'){
                fprintf(stderr,"Error in %d line: move to pit\n",kstr);
                delay();
                err(f,*aa,yr,*ap);
            }
}
//функия для движения
void move(int *ym,int *xm,char ***aa,int yr,int xr,char napr,FILE *f,char ***ap){
            if ((*aa)[*ym][*xm]=='#') (*aa)[*ym][*xm]='_';
            if (napr=='U') *ym=*ym-1;
            else if (napr=='D') *ym=*ym+1;
            else if (napr=='L') *xm=*xm-1;
            else if (napr=='R') *xm=*xm+1;
            check(ym,xm,yr,xr);
            if((*aa)[*ym][*xm]=='^') {
                if (napr=='U') *ym=*ym+1;
                else if (napr=='D') *ym=*ym-1;
                else if (napr=='L') *xm=*xm+1;
                else if (napr=='R') *xm=*xm-1;
                check(ym, xm, yr, xr);
                (*aa)[*ym][*xm]='#';
                fprintf(stderr, "Warning in %d line: MOVE to MOUND\n",kstr);
            }
            else if ((*aa)[*ym][*xm]=='%'){
                fprintf(stderr,"Error in %d line: move to pit\n",kstr);
                delay();
                err(f,*aa,yr,*ap);
            }
            else if ((*aa)[*ym][*xm]=='&'){
                fprintf(stderr,"Warning in %d line: no place to MOVE\n",kstr);
                if (napr=='U') *ym=*ym+1;
                else if (napr=='D') *ym=*ym-1;
                else if (napr=='L') *xm=*xm+1;
                else if (napr=='R') *xm=*xm-1;
                (*aa)[*ym][*xm]='#';
            }
}
//функия для создания ямы
void dig(int ym,int xm,char ***aa,int yr,int xr,char napr){
            int xdig,ydig;
            xdig=xm;
            ydig=ym;
            if (napr=='U') ydig=ym-1;
            else if (napr=='D') ydig=ym+1;
            else if (napr=='L') xdig=xm-1;
            else if (napr=='R') xdig=xm+1;
            check(&ydig,&xdig,yr,xr);
            if ((*aa)[ydig][xdig]=='_') (*aa)[ydig][xdig]='%';
            else fprintf(stderr, "WARNING in %d line: failed to DIG\n",kstr);
}
//функция для создания горы
void mound(int ym,int xm,char ***aa,int yr,int xr,char napr){
            int xmound,ymound;
            xmound=xm;
            ymound=ym;
            if (napr=='U') ymound=ym-1;
            else if (napr=='D') ymound=ym+1;
            else if (napr=='L') xmound=xm-1;
            else if (napr=='R') xmound=xm+1;
            check(&ymound,&xmound,yr,xr);
            if ((*aa)[ymound][xmound]=='_') (*aa)[ymound][xmound]='^';
            else if ((*aa)[ymound][xmound]=='%') (*aa)[ymound][xmound]='_';
            else fprintf(stderr, "WARNING in %d line: failed to MOUND\n",kstr);
}
//функия создания дерева
void grow(int ym,int xm,char ***aa,int yr,int xr,char napr){
            int xgrow,ygrow;
            xgrow=xm;
            ygrow=ym;
            if (napr=='U') ygrow=ym-1;
            else if (napr=='D') ygrow=ym+1;
            else if (napr=='L') xgrow=xm-1;
            else if (napr=='R') xgrow=xm+1;
            check(&ygrow,&xgrow,yr,xr);
            if ((*aa)[ygrow][xgrow]=='_') (*aa)[ygrow][xgrow]='&';
            else fprintf(stderr, "WARNING in %d line: failed to GROW\n",kstr);
}
//функия срубания дерева
void cut(int ym,int xm,char ***aa,int yr,int xr,char napr){
            int xcut,ycut;
            xcut=xm;
            ycut=ym;
            if (napr=='U') ycut=ym-1;
            else if (napr=='D') ycut=ym+1;
            else if (napr=='L') xcut=xm-1;
            else if (napr=='R') xcut=xm+1;
            check(&ycut,&xcut,yr,xr);
            if ((*aa)[ycut][xcut]=='&') (*aa)[ycut][xcut]='_';
            else fprintf(stderr, "WARNING in %d line: failed to CUT\n",kstr);
}
//функия создания камня
void make(int ym,int xm,char ***aa,int yr,int xr,char napr){
            int xmake,ymake;
            xmake=xm;
            ymake=ym;
            if (napr=='U') ymake=ym-1;
            else if (napr=='D') ymake=ym+1;
            else if (napr=='L') xmake=xm-1;
            else if (napr=='R') xmake=xm+1;
            check(&ymake,&xmake,yr,xr);
            if ((*aa)[ymake][xmake]=='_') (*aa)[ymake][xmake]='@';
            else fprintf(stderr, "WARNING in %d line: failed to MAKE\n",kstr);
}
//функия толкания камня
void push(int ym,int xm,char ***aa,int yr,int xr,char napr){
            int xpush,ypush,xrock,yrock;
            xpush=xm;
            ypush=ym;
            xrock=xm;
            yrock=ym;
            if (napr=='U') yrock=ym-1;
            else if (napr=='D') yrock=ym+1;
            else if (napr=='L') xrock=xm-1;
            else if (napr=='R') xrock=xm+1;
            check(&yrock,&xrock,yr,xr);
            if ((*aa)[yrock][xrock]=='@'){
                if (napr=='U') ypush=yrock-1;
                else if (napr=='D') ypush=yrock+1;
                else if (napr=='L') xpush=xrock-1;
                else if (napr=='R') xpush=xrock+1;
                check(&ypush,&xpush,yr,xr);
                if ((*aa)[ypush][xpush]=='&' || (*aa)[ypush][xpush]=='^' || (*aa)[ypush][xpush]=='@') 
                    fprintf(stderr, "WARNING in %d line: failed to PUSH\n",kstr);
                    else if ((*aa)[ypush][xpush]=='%') {(*aa)[ypush][xpush]='_';
                         (*aa)[yrock][xrock]='_';
                    }
                    else {(*aa)[ypush][xpush]='@';
                        (*aa)[yrock][xrock]='_';
                    }
            }
            else fprintf(stderr, "WARNING in %d line: no rock to PUSH\n",kstr);
}
void comand(char *line,int *y0,int *x0,char ***a,int y,int x,FILE *file,char ***ap);
//основная функция которая по строке определяет какую команду нужно сделать
void comand(char *line,int *y0,int *x0,char ***a,int y,int x,FILE *file,char ***ap){
    line[strcspn(line, "\n")] = 0;
    //если стартовая позиция еще не задана то ищем ее дальше
    if (*y0==-1 && *x0==-1) {
        //если старт задан просто строкой
        if (strncmp(line,"START",5)==0){
            if(sscanf(line, "START %d %d",x0,y0)==2) {}
                else {
                    fprintf(stderr, "Error in %d line: no START koordinats\n",kstr);
                    delay();
                    err(file,*a,y-1,*ap);
                }
                if (*x0 < 0 || *y0 < 0 || *x0>=x ||*y0>=y) {
                    fprintf(stderr, "Error in %d line: wrong start koordinats\n",kstr);
                    delay();
                    err(file,*a,y-1,*ap);
                }
                *y0=y-1-*y0;
                (*a)[*y0][*x0]='#';
                print(*a,y,x,*y0,*x0,*ap);
                delay();
                clear();
                clear();
                save_state(*a, *ap, *y0, *x0, y, x);
        }
        //если не нашли то ищем в exec если есть
        else if (strncmp(line,"EXEC",4)==0){
            char ename[100];
            sscanf(line,"EXEC %99s",ename);
            FILE *efile = fopen(ename,"r");
            if (efile == NULL) {
                fprintf(stderr, "Error in %d line: null EXEC file\n",kstr);
                delay();
                err(file,*a,y,*ap);
            }
            char eline[100];
            while (fgets(eline,sizeof(eline),efile)){
                kstr--;
                comand(eline,y0,x0,a,y,x,file,ap);
            }
            if (*x0==-1 && *y0==-1){
                fprintf(stderr, "Error in %d line: no START\n",kstr);
                delay();
                err(file,*a,y,*ap);
                fclose(efile);
            }
            fclose(efile);
        }
        //иначе не нашли стартовые координаты и выдаем ошибку
        else {
            fprintf(stderr, "Error in %d line: no START\n",kstr);
            delay();
            err(file,*a,y,*ap);
        }
    }
    //проверяем если есть пробел перед командой
    else if (strncmp(line, " ",1)==0){
        fprintf(stderr,"Error in %d line: space before comand\n",kstr);
        delay();
        err(file,*a,y,*ap);
    }
    //ошибка если повторный старт
    else if (strncmp(line,"START",5)==0){
        fprintf(stderr,"Error in %d line: other START\n",kstr);
        delay();
        err(file,*a,y,*ap);
    }
    //далее идут несколько простых команд, которые просто перенаправляют нас в соответствующие функции
    else if (strncmp(line,"MOVE",4)==0){
        if (strcmp(line,"MOVE UP")== 0){
            move(y0,x0,a,y,x,'U',file,ap);
        }
        else if (strcmp(line,"MOVE DOWN")==0){
            move(y0,x0,a,y,x,'D',file,ap);
        }
        else if (strcmp(line,"MOVE LEFT")==0){
            move(y0,x0,a,y,x,'L',file,ap);
        }
        else if (strcmp(line,"MOVE RIGHT")==0){
            move(y0,x0,a,y,x,'R',file,ap);
        }
        else fprintf(stderr,"Warning in %d line: incorrect comand MOVE\n",kstr);
        print(*a,y,x,*y0,*x0,*ap);
        delay();
        clear();
        clear();
        save_state(*a, *ap, *y0, *x0, y, x);
    }
    else if (strncmp(line,"DIG",3)==0){
        if (strcmp(line,"DIG UP")==0){
            dig(*y0,*x0,a,y,x,'U');
        }
        else if (strcmp(line,"DIG DOWN")==0){
            dig(*y0,*x0,a,y,x,'D');
        }
        else if (strcmp(line,"DIG LEFT")==0){
            dig(*y0,*x0,a,y,x,'L');
        }
        else if (strcmp(line,"DIG RIGHT")==0){
            dig(*y0,*x0,a,y,x,'R');
        }
        else fprintf(stderr,"Warning in %d line: incorrect comand DIG\n",kstr);
        print(*a,y,x,*y0,*x0,*ap);
        delay();
        clear();
        save_state(*a, *ap, *y0, *x0, y, x);
        }
    else if (strncmp(line,"MOUND",5)==0){
        if (strcmp(line,"MOUND UP")==0){
            mound(*y0,*x0,a,y,x,'U');
        }
        else if (strcmp(line,"MOUND DOWN")==0){
            mound(*y0,*x0,a,y,x,'D');
        }
        else if (strcmp(line,"MOUND LEFT")==0){
            mound(*y0,*x0,a,y,x,'L');
        }
        else if (strcmp(line,"MOUND RIGHT")==0){
            mound(*y0,*x0,a,y,x,'R');
        }
        else fprintf(stderr,"Warning in %d line: incorrect comand MOUND\n",kstr);
        print(*a,y,x,*y0,*x0,*ap);
        delay();
        clear();
        save_state(*a, *ap, *y0, *x0, y, x);
    }
    else if (strncmp(line,"PAINT",5)==0){
        char bukva;
        if (sscanf(line,"PAINT %c",&bukva)==1)
            if (bukva<='z' && bukva>='a'){
                (*ap)[*y0][*x0]=bukva;
                print(*a,y,x,*y0,*x0,*ap);
                delay();
                clear();
                save_state(*a, *ap, *y0, *x0, y, x);
            }
            else fprintf(stderr,"Warning in %d line: wrong letter in PAINT\n",kstr);
        else fprintf(stderr,"Warning in %d line: failed to read letter in PAINT\n",kstr);
    }
    else if (strncmp(line,"JUMP",4)==0){
        int kjump;
        if (strncmp(line, "JUMP UP",7)==0){
            sscanf(line,"JUMP UP %d",&kjump);
            jump(kjump,y0,x0,a,y,x,'U',file,ap);
        }
        else if (strncmp(line, "JUMP DOWN",9)==0){
            sscanf(line,"JUMP DOWN %d",&kjump);
            jump(kjump,y0,x0,a,y,x,'D',file,ap);
        }
        else if (strncmp(line, "JUMP LEFT",9)==0){
            sscanf(line,"JUMP LEFT %d",&kjump);
            jump(kjump,y0,x0,a,y,x,'L',file,ap);
        }
        else if (strncmp(line, "JUMP RIGHT",10)==0){
            sscanf(line,"JUMP RIGHT %d",&kjump);
            jump(kjump,y0,x0,a,y,x,'R',file,ap);
        }
        else fprintf(stderr,"Warning in %d line: incorrect comand JUMP\n",kstr);
        print(*a,y,x,*y0,*x0,*ap);
        delay();
        clear();
        clear();
        save_state(*a, *ap, *y0, *x0, y, x);
        }
    else if (strncmp(line,"GROW",4)==0){
        if (strcmp(line,"GROW UP")==0){
            grow(*y0,*x0,a,y,x,'U');
        }
        else if (strcmp(line,"GROW DOWN")==0){
            grow(*y0,*x0,a,y,x,'D');
        }
        else if (strcmp(line,"GROW LEFT")==0){
            grow(*y0,*x0,a,y,x,'L');
        }
        else if (strcmp(line,"GROW RIGHT")==0){
            grow(*y0,*x0,a,y,x,'R');
        }
        else fprintf(stderr,"Warning in %d line: incorrect comand GROW\n",kstr);
        print(*a,y,x,*y0,*x0,*ap);
        delay();
        clear();
        clear();
        save_state(*a, *ap, *y0, *x0, y, x);
    }
    else if (strncmp(line,"CUT",3)==0){
        if (strcmp(line,"CUT UP")==0){
            cut(*y0,*x0,a,y,x,'U');
        }
        else if (strcmp(line,"CUT DOWN")==0){
            cut(*y0,*x0,a,y,x,'D');
        }
        else if (strcmp(line,"CUT LEFT")==0){
            cut(*y0,*x0,a,y,x,'L');
        }
        else if (strcmp(line,"CUT RIGHT")==0){
            cut(*y0,*x0,a,y,x,'R');
        }
        else fprintf(stderr,"Warning in %d line: incorrect comand CUT\n",kstr);
        print(*a,y,x,*y0,*x0,*ap);
        delay();
        clear();
        clear();
        save_state(*a, *ap, *y0, *x0, y, x);
    }
    else if (strncmp(line,"MAKE",4)==0){
        if (strcmp(line,"MAKE UP")==0){
            make(*y0,*x0,a,y,x,'U');
        }
        else if (strcmp(line,"MAKE DOWN")==0){
            make(*y0,*x0,a,y,x,'D');
        }
        else if (strcmp(line,"MAKE LEFT")==0){
            make(*y0,*x0,a,y,x,'L');
        }
        else if (strcmp(line,"MAKE RIGHT")==0){
            make(*y0,*x0,a,y,x,'R');
        }
        else fprintf(stderr,"Warning in %d line: incorrect comand MAKE\n",kstr);
        print(*a,y,x,*y0,*x0,*ap);
        delay();
        clear();
        clear();
        save_state(*a, *ap, *y0, *x0, y, x);
    }
    else if (strncmp(line,"PUSH",4)==0){
        if (strcmp(line,"PUSH UP")==0){
            push(*y0,*x0,a,y,x,'U');
        }
        else if (strcmp(line,"PUSH DOWN")==0){
            push(*y0,*x0,a,y,x,'D');
        }
        else if (strcmp(line,"PUSH LEFT")==0){
            push(*y0,*x0,a,y,x,'L');
        }
        else if (strcmp(line,"PUSH RIGHT")==0){
            push(*y0,*x0,a,y,x,'R');
        }
        else fprintf(stderr,"Warning in %d line: incorrect comand PUSH\n",kstr);
        print(*a,y,x,*y0,*x0,*ap);
        delay();
        clear();
        clear();
        save_state(*a, *ap, *y0, *x0, y, x);
    }
    //комментарии пропускаем
    else if (strncmp(line,"//",2)==0){}
    //повторный load приводит к ошибке
    else if (strncmp(line,"LOAD",4)==0){
        fprintf(stderr,"Error in %d line: incorrect LOAD\n",kstr);
        delay();
        err(file,*a,y,*ap);
    }
    //повторный size приводит к ошибке
    else if (strncmp(line,"SIZE",4)==0){
        fprintf(stderr,"Error in %d line: other SIZE\n",kstr);
        delay();
        err(file,*a,y,*ap);
    }
    //проверяем условную команду и если все правильно то выполнем команду которая написана
    else if (strncmp(line,"IF CELL",4)==0){
        save_state(*a, *ap, *y0, *x0, y, x);
        int xif=0,yif=0;
        char sif[100];
        char cif;
        if (sscanf(line,"IF CELL %d %d IS %c THEN %99[^\n]",&xif,&yif,&cif,sif)==4){
            if ((*a)[yif][xif]==cif){
                comand(sif,y0,x0,a,y,x,file,ap);
            }
        }
        else fprintf(stderr,"Warning in %d line: incorrect comand CELL\n",kstr);
        delay();
    }
    //находим файл exec и делаем из него все команды
    else if (strncmp(line, "EXEC",4) == 0){
        char ename[100];
        sscanf(line,"EXEC %99s",ename);
        FILE *efile = fopen(ename,"r");
        if (efile == NULL) {
            fprintf(stderr, "Error in %d line: cannot open EXEC file\n",kstr);
            return;
        }
        char eline[100];
        while (fgets(eline,sizeof(eline),efile)!=NULL){
            kstr--;
            comand(eline,y0,x0,a,y,x,file,ap);
        }
        fclose(efile);
    }
    //возвращаем предыдущее состояние поля
    else if (strncmp(line, "UNDO", 4) == 0) {
        if (current_state > 0) {
            current_state--;
            load_state(current_state, a, ap, y0, x0, y, x);
            print(*a, y, x, *y0, *x0, *ap);
            delay();
            clear();
            clear();
            save_state(*a, *ap, *y0, *x0, y, x);
        } else {
            fprintf(stderr, "Warning in %d line: no states to undo\n",kstr);
            delay();
        };
    }
    //остальные команды считаем неверными
    else {
        fprintf(stderr,"Error in %d line: incorrect comand\n",kstr);
        delay();
        err(file,*a,y,*ap);
    }
}
//основная часть программы
int main(int argc, char *argv[]){
    current_state = -1;
    total_states = 0;
    memset(undo_list, 0, sizeof(undo_list));
    char *input_filename=NULL;
    char *output_filename=NULL;
    //анализируем доп параметры если заданы при запуске
    for (int i=1;i<argc;i++) {
        if (strcmp(argv[i],"no-display")==0){
            display=0;
        }
        else if (strcmp(argv[i],"no-save")==0){
            save=0;
        }
        else if (strcmp(argv[i],"interval")==0 && i+1<argc){
            interval=atof(argv[++i]);
            if (interval<0) {
                fprintf(stderr,"Error: incorrect interval");
                delay();
                return(1);
            }
        }
        else if (input_filename==NULL){
            input_filename=argv[i];
        }
        else if(output_filename==NULL){
            output_filename=argv[i];
        }
    }
    if (input_filename == NULL || output_filename == NULL) {
        fprintf(stderr, "Error: files has not found\n");
        delay();
        return 1;
    }
    FILE *file=fopen(input_filename,"r");
    if (file==NULL) {
        fprintf(stderr, "Error: input file is null\n");
        delay();
        return 1;
    }
    char line[100];
    int x=0,y=0;
    int x0=-1,y0=-1;
    char **a=malloc(1 * sizeof(char*));
    char **ap=malloc(1 * sizeof(char*));
    fgets(line, sizeof(line), file);
    kstr++;
    //если первая строчка load то считываем поле из файла, если есть покраска клеток то записываем это в дублирующий массив
    if (strncmp(line,"LOAD",4)==0){
        char lload[100];
        if (sscanf(line,"LOAD %99s",lload)==1){}
        else {
            fprintf(stderr, "Error in %d line: no file to LOAD\n",kstr);
            delay();
            fclose(file);
            free(a);
            free(ap);
            return 1;
        }
        FILE *lfile=fopen(lload,"r");
        if (lfile==NULL){
            fprintf(stderr, "Error: LOAD file is NULL\n");
            delay();
            fclose(file);
            free(a);
            free(ap);
            return 1;
        }
        char lline[100];
        y=0;
        int kxstart=0;
        while (fgets(lline, sizeof(lline),lfile)){
            lline[strcspn(lline,"\n")]=0;
            y++;
            a=realloc(a,(y)*sizeof(char*));
            ap=realloc(ap,(y)*sizeof(char*));
            x=strlen(lline);
            a[y-1]=malloc((x+1)*sizeof(char));
            ap[y-1]=malloc((x+1)*sizeof(char));
            for(int i=0;i<x;i++){
                if (lline[i]=='_' || lline[i]=='#' || lline[i]=='@' || lline[i]=='%' || lline[i]=='^' || lline[i]=='&' || (lline[i]>='a' && lline[i]<='z')){
                    a[y-1][i]=lline[i];
                    if (lline[i]>='a' && lline[i]<='z'){
                        ap[y-1][i]=lline[i];
                    }
                    else ap[y-1][i]='_';
                    if (lline[i]=='#'){
                        kxstart++;
                        y0=y-1;
                        x0=i;
                    }
                }
                else {
                    fprintf(stderr, "Error in %d line: incorrect LOAD file\n",kstr);
                    delay();
                    fclose(lfile);
                    err(file,a,y,ap);
                }
            }
            for(int i=0;i<y-1;i++){
                if(strlen(a[i])!=strlen(a[i+1])){
                    fprintf(stderr, "Error in %d line: incorrect LOAD file\n",kstr);
                    delay();
                    fclose(lfile);
                    err(file,a,y,ap);
                }
            }
            a[y-1][x] = '\0';
        }
        save_state(a, ap, y0, x0, y, x);
        print(a,y,x,y0,x0,ap);
        delay();
        clear();
        if(kxstart!=1){
            fprintf(stderr, "Error in %d line: incorrect START kordinats in LOAD\n",kstr);
            delay();
            fclose(lfile);
            err(file,a,y,ap);
        }
    }
    //иначе считваем размер поля и создаем его, а также создаем дублирующее поле для покраски
    else {
        if (sscanf(line,"SIZE %d %d",&x,&y) == 2) {}
        else {
            fprintf(stderr, "Error in %d line: no SIZE\n",kstr);
            fclose(file);
            return 1;
        }
        if (x <= 0 || y <= 0) {
            fprintf(stderr,"Error in %d line: wrong SIZE\n",kstr);
            fclose(file);
            return 1;
        }
        a=realloc(a,y*sizeof(char*));
        ap=realloc(ap,y*sizeof(char*));
        for(int i=0;i<y;i++) {
            a[i]=malloc(x*sizeof(char));
            ap[i]=malloc(x*sizeof(char));
        }
        for (int i=0;i<y;i++){
            for (int j=0;j<x;j++){
                a[i][j]='_';
                ap[i][j]='_';
            }
        }
    }
    clear();
    //построчно считываем команды с файла
    while (fgets(line, sizeof(line),file)) {
        kstr++;
        comand(line,&y0,&x0,&a,y,x,file,&ap);
    }
    clear();
    //сохраняем в файл если нужно
    if (save==1){
        FILE *output=fopen(output_filename,"w");
        if (output==NULL){
        fprintf(stderr, "ERROR: no output file information\n");
    }
        else for (int i=0;i<y;i++){
            for (int j=0;j<x;j++){
                if (ap[i][j]=='_'){
                if (i==y0 && j==x0) fprintf(output,"#");
                else fprintf(output,"%c",a[i][j]);
                }
                else fprintf(output,"%c",ap[i][j]);
            }
        if (i!=y-1)fprintf(output,"\n");
        }
        fclose(output);
    }   
    //освобождаем память и завершаем программу
    for(int i = 0; i < y; i++) {
        free(a[i]);
    }
    free(a);
    fclose(file);
    return 0;
}