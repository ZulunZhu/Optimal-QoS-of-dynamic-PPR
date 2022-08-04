package main

/*
#include "mylib_tx.h"
*/
import "C"
import (
	"fmt"
)

func main() {
	ret := C.program_stop()
	fmt.Println(ret)
}
