#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif
int display=1;
int save=1;
int interval=1.0;
typedef struct {
    char **field;
    char **paint;
    int x, y;
} State;
static State undo_list[100];
static int current_state = -1;
static int total_states = 0;
void clear(){
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}
void delay(){
#ifdef _WIN32
    Sleep((int)(interval * 1000));
#else
    sleep((int)interval);
#endif
}
char** copy_array(char **src, int rows, int cols) {
    char **copy = (char**)malloc(rows * sizeof(char*));
    for (int i = 0; i < rows; i++) {
        copy[i] = (char*)malloc(cols * sizeof(char));
        memcpy(copy[i], src[i], cols);
    }
    return copy;
}
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
    
    current_state = total_states;
    total_states++;
}
void free_state(State *state, int rows) {
    if (state->field) {
        for (int i = 0; i < rows; i++) free(state->field[i]);
        free(state->field);
    }
    if (state->paint) {
        for (int i = 0; i < rows; i++) free(state->paint[i]);
        free(state->paint);
    }
}
void load_state(int index, char ***field, char ***paint, int *y, int *x, int field_y, int field_x) {
    if (index < 0 || index >= total_states) return;
    for (int i = 0; i < field_y; i++) {
        free((*field)[i]);
        free((*paint)[i]);
    }
    free(*field);
    free(*paint);
    *field = copy_array(undo_list[index].field, field_y, field_x);
    *paint = copy_array(undo_list[index].paint, field_y, field_x);
    *y = undo_list[index].y;
    *x = undo_list[index].x;
    current_state = index;
}
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
void check(int *ych,int *xch,int yr,int xr){
    if (*xch>=xr) *xch=0;
    if (*ych>=yr) *ych=0;
    if (*xch<0) *xch=xr-1;
    if (*ych<0) *ych=yr-1;
}
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
                fprintf(stderr, "Warning: failed to JUMP\n");
                break;
            }
            }
            if ((*aa)[*ym][*xm]=='%'){
                fprintf(stderr,"Error: move to pit\n");
                err(f,*aa,yr,*ap);
            }
}
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
                fprintf(stderr, "Warning: MOVE to MOUND\n");
            }
            else if ((*aa)[*ym][*xm]=='%'){
                fprintf(stderr,"Error: move to pit\n");
                err(f,*aa,yr,*ap);
            }
            else if ((*aa)[*ym][*xm]=='&'){
                fprintf(stderr,"Warning: no place to MOVE\n");
                if (napr=='U') *ym=*ym+1;
                else if (napr=='D') *ym=*ym-1;
                else if (napr=='L') *xm=*xm+1;
                else if (napr=='R') *xm=*xm-1;
                (*aa)[*ym][*xm]='#';
            }
}
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
            else fprintf(stderr, "WARNING: failed to DIG\n");
}
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
            else fprintf(stderr, "WARNING: failed to MOUND\n");
}
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
            else fprintf(stderr, "WARNING: failed to GROW\n");
}
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
            else fprintf(stderr, "WARNING: failed to CUT\n");
}
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
            else fprintf(stderr, "WARNING: failed to MAKE\n");
}
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
                    fprintf(stderr, "WARNING: failed to PUSH\n");
                    else if ((*aa)[ypush][xpush]=='%') {(*aa)[ypush][xpush]='_';
                         (*aa)[yrock][xrock]='_';
                    }
                    else {(*aa)[ypush][xpush]='@';
                        (*aa)[yrock][xrock]='_';
                    }
            }
            else fprintf(stderr, "WARNING: no rock to PUSH\n");
}
void comand(char *line,int *y0,int *x0,char ***a,int y,int x,FILE *file,char ***ap);
void comand(char *line,int *y0,int *x0,char ***a,int y,int x,FILE *file,char ***ap){
    line[strcspn(line, "\n")] = 0;
    if (*y0==-1 && *x0==-1) {
        if (strncmp(line,"START",5)==0){
            if(sscanf(line, "START %d %d",x0,y0)==2) {}
                else {
                    fprintf(stderr, "Error: no START koordinats\n");
                    err(file,*a,y-1,*ap);
                }
                if (*x0 < 0 || *y0 < 0 || *x0>=x ||*y0>=y) {
                    fprintf(stderr, "Error: wrong start koordinats\n");
                    err(file,*a,y-1,*ap);
                }
                *y0=y-1-*y0;
                (*a)[*y0][*x0]='#';
                print(*a,y,x,*y0,*x0,*ap);
                delay();
                clear();
                save_state(*a, *ap, *y0, *x0, y, x);
        }
        else if (strncmp(line,"EXEC",4)==0){
            char ename[100];
            sscanf(line,"EXEC %99s",ename);
            FILE *efile = fopen(ename,"r");
            if (efile == NULL) {
                fprintf(stderr, "Error: null EXEC file\n");
                err(file,*a,y,*ap);
            }
            char eline[100];
            while (fgets(eline,sizeof(eline),efile)){
                comand(eline,y0,x0,a,y,x,file,ap);
            }
            if (*x0==-1 && *y0==-1){
                fprintf(stderr, "Error: no START\n");
                err(file,*a,y,*ap);
                fclose(efile);
            }
            fclose(efile);
        }
        else {
        fprintf(stderr, "Error: no START\n");
        err(file,*a,y,*ap);
        }
    }
    else if (strncmp(line, " ",1)==0){
        fprintf(stderr,"Error: space before comand\n");
        err(file,*a,y,*ap);
    }
    else if (strncmp(line,"START",5)==0){
        fprintf(stderr,"Error: other START\n");
        err(file,*a,y,*ap);
    }
    else if (strncmp(line,"MOVE",4)==0){
        save_state(*a, *ap, *y0, *x0, y, x);
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
        else fprintf(stderr,"Warning error comand MOVE\n");
        clear();
        print(*a,y,x,*y0,*x0,*ap);
        delay();
        clear();
        clear();
    }
    else if (strncmp(line,"DIG",3)==0){
        save_state(*a, *ap, *y0, *x0, y, x);
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
        else fprintf(stderr,"Warning error comand DIG\n");
        print(*a,y,x,*y0,*x0,*ap);
        delay();
        clear();
        }
    else if (strncmp(line,"MOUND",5)==0){
        save_state(*a, *ap, *y0, *x0, y, x);
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
        else fprintf(stderr,"Warning error comand MOUND\n");
        print(*a,y,x,*y0,*x0,*ap);
        delay();
        clear();
    }
    else if (strncmp(line,"PAINT",5)==0){
        save_state(*a, *ap, *y0, *x0, y, x);
        char bukva;
        if (sscanf(line,"PAINT %c",&bukva)==1)
            if (bukva<='z' && bukva>='a'){
                (*ap)[*y0][*x0]=bukva;
                print(*a,y,x,*y0,*x0,*ap);
                delay();
                clear();
            }
            else fprintf(stderr,"Warning: wrong letter in PAINT\n");
        else fprintf(stderr,"Warning: failed to read letter in PAINT\n");
    }
    else if (strncmp(line,"JUMP",4)==0){
        save_state(*a, *ap, *y0, *x0, y, x);
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
        else fprintf(stderr,"Warning error comand JUMP\n");
        print(*a,y,x,*y0,*x0,*ap);
        delay();
        clear();
        }
    else if (strncmp(line,"GROW",4)==0){
        save_state(*a, *ap, *y0, *x0, y, x);
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
        else fprintf(stderr,"Warning error comand GROW\n");
        print(*a,y,x,*y0,*x0,*ap);
        delay();
        clear();
    }
    else if (strncmp(line,"CUT",3)==0){
        save_state(*a, *ap, *y0, *x0, y, x);
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
        else fprintf(stderr,"Warning error comand CUT\n");
        print(*a,y,x,*y0,*x0,*ap);
        delay();
        clear();
    }
    else if (strncmp(line,"MAKE",4)==0){
        save_state(*a, *ap, *y0, *x0, y, x);
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
        else fprintf(stderr,"Warning error comand MAKE\n");
        print(*a,y,x,*y0,*x0,*ap);
        delay();
        clear();
    }
    else if (strncmp(line,"PUSH",4)==0){
        save_state(*a, *ap, *y0, *x0, y, x);
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
        else fprintf(stderr,"Warning error comand PUSH\n");
        print(*a,y,x,*y0,*x0,*ap);
        delay();
        clear();
    }
    else if (strncmp(line,"//",2)==0){}
    else if (strncmp(line,"LOAD",4)==0){
        fprintf(stderr,"Error: incorrect LOAD\n");
        err(file,*a,y,*ap);
    }
    else if (strncmp(line,"SIZE",4)==0){
        fprintf(stderr,"Error: other SIZE\n");
        err(file,*a,y,*ap);
    }
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
        else fprintf(stderr,"Warning: incorrect comand CELL\n");
    }
    else if (strncmp(line, "EXEC",4) == 0){
        char ename[100];
        sscanf(line,"EXEC %99s",ename);
        FILE *efile = fopen(ename,"r");
        if (efile == NULL) {
            fprintf(stderr, "Error: cannot open EXEC file\n");
            return;
        }
        char eline[100];
        while (fgets(eline,sizeof(eline),efile)!=NULL){
            comand(eline,y0,x0,a,y,x,file,ap);
        }
        fclose(efile);
    }
    else if (strncmp(line, "UNDO", 4) == 0) {
    if (current_state > 0) {
        load_state(current_state - 1, a, ap, y0, x0, y, x);
        print(*a, y, x, *y0, *x0, *ap);
        delay();
        clear();
        clear();
    } else {
        fprintf(stderr, "Warning: no states to undo\n");
    }
}
    else {
        fprintf(stderr,"Error: incorrect comand\n");
        err(file,*a,y,*ap);
    }
}
int main(int argc, char *argv[]){
    current_state = -1;
    total_states = 0;
    memset(undo_list, 0, sizeof(undo_list));
    char *input_filename=NULL;
    char *output_filename=NULL;
    for (int i=1;i<argc;i++) {
        if (strcmp(argv[i],"no-display")==0){
            display=0;
        }
        else if (strcmp(argv[i],"no-save")==0){
            save=0;
        }
        else if (strcmp(argv[i],"interval")==0 && i+1<argc){
            interval=atof(argv[++i]);
            if (interval<=0) {
                fprintf(stderr,"Error: incorrect interval");
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
        return 1;
    }
    FILE *file=fopen(input_filename,"r");
    if (file==NULL) {
        perror("Error: input file is null\n");
        return 1;
    }
    char line[100];
    int x=0,y=0;
    int x0=-1,y0=-1;
    char **a=malloc(1 * sizeof(char*));
    char **ap=malloc(1 * sizeof(char*));
    fgets(line, sizeof(line), file);
    if (strncmp(line,"LOAD",4)==0){
        char lload[100];
        if (sscanf(line,"LOAD %99s",lload)==1){}
        else {
            fprintf(stderr, "Error: no file to LOAD\n");
            fclose(file);
            free(a);
            return 1;
        }
        FILE *lfile=fopen(lload,"r");
        if (lfile==NULL){
            fprintf(stderr, "Error: LOAD file is NULL\n");
            fclose(file);
            free(a);
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
                    fprintf(stderr, "Error: incorrect LOAD file\n");
                    fclose(lfile);
                    err(file,a,y,ap);
                }
            }
            save_state(a, ap, y0, x0, y, x);
            print(a,y,x,y0,x0,ap);
            delay();
            clear();
            for(int i=0;i<y-1;i++){
                if(strlen(a[i])!=strlen(a[i+1])){
                    fprintf(stderr, "Error: incorrect LOAD file\n");
                    fclose(lfile);
                    err(file,a,y,ap);
                }
            }
            a[y-1][x] = '\0';
        }
        if(kxstart!=1){
            fprintf(stderr, "Error: incorrect START kordinats in LOAD\n");
            fclose(lfile);
            err(file,a,y,ap);
        }
    }
    else {
        if (sscanf(line,"SIZE %d %d",&x,&y) == 2) {}
        else {
            fprintf(stderr, "Error: no SIZE\n");
            fclose(file);
            return 1;
        }
        if (x <= 0 || y <= 0) {
        fprintf(stderr,"Error: wrong SIZE\n");
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
    while (fgets(line, sizeof(line),file)) {
        comand(line,&y0,&x0,&a,y,x,file,&ap);
    }
    clear();
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
    for(int i = 0; i < y; i++) {
        free(a[i]);
    }
    free(a);
    fclose(file);
    return 0;
}