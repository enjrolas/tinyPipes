#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char** split(const char *str, const char *delimiter, size_t *len){
    char *text, *p, *first, **array;
    int c;
    char** ret;

    *len = 0;
    text=strdup(str);
    if(text==NULL) return NULL;
    for(c=0,p=text;NULL!=(p=strtok(p, delimiter));p=NULL, c++)//count item
        if(c==0) first=p; //first token top

    ret=(char**)malloc(sizeof(char*)*c+1);//+1 for NULL
    if(ret==NULL){
        free(text);
        return NULL;
    }
    strcpy(text, str+(first-text));//skip until top token
    array=ret;

    for(p=text;NULL!=(p=strtok(p, delimiter));p=NULL){
        *array++=p;
    }
    *array=NULL;
    *len=c;
    return ret;
}

void free4split(char** sa){
    char **array=sa;

    if(sa!=NULL){
        free(array[0]);//for text
        free(sa);      //for array
    }
}

