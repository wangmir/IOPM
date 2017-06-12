#!/bin/bash

#./iopm.exe $1 -mtb $2 -cut 40 -ss 64 -op 6 -pas 512 -pac 256 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe $1 -mtb $2 -cut 40 -ss 64 -op 6 -pas 512 -pac 224 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe $1 -mtb $2 -cut 40 -ss 64 -op 6 -pas 512 -pac 192 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe $1 -mtb $2 -cut 40 -ss 64 -op 6 -pas 256 -pac 512 -cs 256 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
./iopm.exe $1 -mtb $2 -cut 40 -ss 16 -op 2 -pas 256 -pac 112 -cs 256 -stc 4 -msr 1 -seqr 0.5 -randm 0.7 -randr 0.3 -randi 1 -rands 1
#./iopm.exe $1 -mtb $2 -cut 40 -ss 64 -op 6 -pas 256 -pac 392 -cs 256 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64

#./iopm.exe $1 -cut 40 -ss 64 -op 6 -pas 1024 -pac 128 -cs 1024 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe $1 -cut 40 -ss 64 -op 6 -pas 1024 -pac 112 -cs 1024 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe $1 -cut 40 -ss 64 -op 6 -pas 1024 -pac 96 -cs 1024 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64

#./iopm.exe trace/$1 -pas 512 -pac 128 -cs 2048 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 512 -pac 112 -cs 2048 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 512 -pac 96 -cs 2048 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 512 -pac 128 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 512 -pac 112 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 512 -pac 96 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 512 -pac 128 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 512 -pac 112 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 512 -pac 96 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64

#./iopm.exe trace/$1 -pas 256 -pac 256 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 256 -pac 224 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 256 -pac 192 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 256 -pac 256 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 256 -pac 224 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 256 -pac 192 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 256 -pac 256 -cs 256 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 256 -pac 224 -cs 256 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 256 -pac 192 -cs 256 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64

#./iopm.exe trace/$1 -pas 128 -pac 512 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 128 -pac 448 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 128 -pac 384 -cs 512 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 128 -pac 512 -cs 256 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 128 -pac 448 -cs 256 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 128 -pac 384 -cs 256 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 128 -pac 512 -cs 128 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 128 -pac 448 -cs 128 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64
#./iopm.exe trace/$1 -pas 128 -pac 384 -cs 128 -stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 64

