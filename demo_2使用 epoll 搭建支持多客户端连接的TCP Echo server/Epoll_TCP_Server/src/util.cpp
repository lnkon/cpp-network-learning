#include "../include/util.hpp"
#include <stdlib.h>

void errif(bool condition, const std::string errmsg){
    if(condition){
        perror(errmsg.c_str());
        exit(EXIT_FAILURE);
    }
}