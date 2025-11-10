int selstep(void *parr, int eltsize, int numelts, int nsorted, cmp_t cmp) {
    if (nsorted>=numelts-1) return 0;
    char *arr = (char *)parr;
    int min_index=nsorted;
    for (int i=nsorted+1;i<numelts; i++) {
        if (cmp(arr + i * eltsize, arr + min_index * eltsize)) {
            min_index=i;
        }
    }
    if (min_index != nsorted) {
        char *temp=malloc(eltsize);
        if (!temp) return -1;
        memcpy(temp, arr + nsorted * eltsize, eltsize);
        memcpy(arr + nsorted * eltsize, arr + min_index * eltsize, eltsize);
        memcpy(arr + min_index * eltsize, temp, eltsize);
        free(temp);
    }
    return 0;
}